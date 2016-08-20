#include <libxsmm_timer.h>

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#if defined(__MKL) || defined(MKL_DIRECT_CALL_SEQ) || defined(MKL_DIRECT_CALL)
# include <mkl_trans.h>
#elif defined(__OPENBLAS)
# include <openblas/cblas.h>
#endif
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

#if !defined(USE_SELF_VALIDATION) \
 && !defined(__MKL) && !defined(MKL_DIRECT_CALL_SEQ) && !defined(MKL_DIRECT_CALL) \
 && !defined(__OPENBLAS)
# define USE_SELF_VALIDATION
#endif

#if !defined(REAL_TYPE)
# define REAL_TYPE double
#endif

#if !defined(USE_LIBXSMM_MALLOC)
# define USE_LIBXSMM_MALLOC
#endif

/**
 * This uses an internal unsupported API; not for production code!
 * Perhaps this makes a case for an alternative allocator...
 */
#if defined(USE_LIBXSMM_MALLOC)
LIBXSMM_API void* libxsmm_malloc(size_t size);
LIBXSMM_API void libxsmm_free(const void* memory);
# define MALLOC(SIZE) libxsmm_malloc(SIZE)
# define FREE(BUFFER) libxsmm_free(BUFFER)
#else
# define MALLOC(SIZE) malloc(SIZE)
# define FREE(BUFFER) free(BUFFER)
#endif


LIBXSMM_INLINE LIBXSMM_RETARGETABLE REAL_TYPE initial_value(libxsmm_blasint i, libxsmm_blasint j, libxsmm_blasint ld)
{
  return (REAL_TYPE)(i * ld + j);
}


int main(int argc, char* argv[])
{
  const char t = 1 < argc ? *argv[1] : 'o';
  const libxsmm_blasint m = 2 < argc ? atoi(argv[2]) : 4096;
  const libxsmm_blasint n = 3 < argc ? atoi(argv[3]) : m;
  const libxsmm_blasint lda = LIBXSMM_MAX/*sanitize ld*/(4 < argc ? atoi(argv[4]) : 0, m);
  const libxsmm_blasint ldb = LIBXSMM_MAX/*sanitize ld*/(5 < argc ? atoi(argv[5]) : 0, n);

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload target(LIBXSMM_OFFLOAD_TARGET)
#endif
  {
    REAL_TYPE *const a = (REAL_TYPE*)MALLOC(lda * (('o' == t || 'O' == t) ? n : lda) * sizeof(REAL_TYPE));
    REAL_TYPE *const b = (REAL_TYPE*)MALLOC(ldb * (('o' == t || 'O' == t) ? m : ldb) * sizeof(REAL_TYPE));
    /* validate against result computed by Intel MKL */
#if !defined(USE_SELF_VALIDATION)
    REAL_TYPE *const c = (REAL_TYPE*)MALLOC(ldb * (('o' == t || 'O' == t) ? m : ldb) * sizeof(REAL_TYPE));
#endif
    const unsigned int size = m * n * sizeof(REAL_TYPE);
    unsigned long long start;
    libxsmm_blasint i = 0, j;
    double duration;

    fprintf(stdout, "m=%i n=%i lda=%i ldb=%i size=%.fMB (%s, %s)\n", m, n, lda, ldb,
      1.0 * size / (1 << 20), 8 == sizeof(REAL_TYPE) ? "DP" : "SP",
      ('o' == t || 'O' == t) ? "out-of-place" : "in-place");

    for (i = 0; i < n; ++i) {
      for (j = 0; j < m; ++j) {
        a[i*lda+j] = initial_value(i, j, m);
      }
    }

    if (('o' == t || 'O' == t)) {
      start = libxsmm_timer_tick();
      libxsmm_otrans_omp(b, a, sizeof(REAL_TYPE), m, n, lda, ldb);
#if defined(USE_SELF_VALIDATION)
      /* without Intel MKL, construct an invariant result and check against it */
      libxsmm_otrans(a, b, sizeof(REAL_TYPE), n, m, ldb, lda);
#endif
      duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
    }
    else {
      if (('i' != t && 'I' != t)) {
        fprintf(stderr, "In-place transpose assumed!\n");
      }
      start = libxsmm_timer_tick();
      /*libxsmm_itrans(a, sizeof(REAL_TYPE), m, n, lda);*/
#if defined(USE_SELF_VALIDATION)
      /* without Intel MKL, construct an invariant result and check against it */
      /*libxsmm_itrans(a, sizeof(REAL_TYPE), n, m, lda);*/
#endif
      duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
    }

#if defined(USE_SELF_VALIDATION)
    /* without Intel MKL, check against a known result (invariant) */
    for (i = 0; i < n; ++i) {
      for (j = 0; j < m; ++j) {
        if (initial_value(i, j, m) != a[i*lda+j]) {
          i = n + 1; /* leave outer loop as well */
          break;
        }
      }
    }
#endif

    if (i <= n) {
      if (0 < duration) {
        fprintf(stdout, "\tbandwidth: %.1f GB/s\n", size / (duration * (1 << 30)));
      }
      fprintf(stdout, "\tduration: %.0f ms\n", 1000.0 * duration);
    }
#if defined(USE_SELF_VALIDATION)
    else {
      fprintf(stderr, "Validation failed!\n");
    }
#else /* Intel MKL or OpenBLAS transpose interface available */
    {
      double duration2;
      if (('o' == t || 'O' == t)) {
        start = libxsmm_timer_tick();
#if defined(__MKL) || defined(MKL_DIRECT_CALL_SEQ) || defined(MKL_DIRECT_CALL)
        LIBXSMM_CONCATENATE(mkl_, LIBXSMM_TPREFIX(REAL_TYPE, omatcopy))('C', 'T', m, n, 1/*alpha*/, a, lda, c, ldb);
#elif defined(__OPENBLAS) /* tranposes are not really covered by the common CBLAS interface */
        LIBXSMM_CONCATENATE(cblas_, LIBXSMM_TPREFIX(REAL_TYPE, omatcopy))(CblasColMajor, CblasTrans, m, n, 1/*alpha*/, a, lda, c, ldb);
#else
#       error No alternative transpose routine available!
#endif
        duration2 = libxsmm_timer_duration(start, libxsmm_timer_tick());
      }
      else {
        start = libxsmm_timer_tick();
        /* TODO: call in-place variant */
        duration2 = libxsmm_timer_duration(start, libxsmm_timer_tick());
      }
      /* validate against result computed by alternative routine */
      for (i = 0; i < m; ++i) {
        for (j = 0; j < n; ++j) {
          if (b[i*ldb+j] != c[i*ldb+j]) {
            i = m + 1; /* leave outer loop as well */
            break;
          }
        }
      }
      if (i <= m) {
        if (0 < duration2) {
          fprintf(stdout, "\treference: %.1fx\n", duration / duration2);
        }
      }
      else {
        fprintf(stderr, "Validation failed!\n");
      }
    }
    FREE(c);
#endif
    FREE(a);
    FREE(b);
  }
  return EXIT_SUCCESS;
}

