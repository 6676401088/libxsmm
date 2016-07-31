#include <libxsmm.h>
#include "libxsmm_intrinsics_x86.h"

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <stdio.h>
#if !defined(NDEBUG)
# include <assert.h>
#endif
#if defined(__MKL) || defined(MKL_DIRECT_CALL_SEQ) || defined(MKL_DIRECT_CALL)
# include <mkl_trans.h>
#endif
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

#if !defined(LIBXSMM_TRANSPOSE_CACHESIZE)
# define LIBXSMM_TRANSPOSE_CACHESIZE 32768
#endif
#if !defined(LIBXSMM_TRANSPOSE_CHUNK)
# define LIBXSMM_TRANSPOSE_CHUNK 32
#endif
#if !defined(LIBXSMM_TRANSPOSE_CONSECUTIVE_STORE)
# define LIBXSMM_TRANSPOSE_CONSECUTIVE_STORE
#endif

/* consecutive store and strided load */
#if defined(LIBXSMM_TRANSPOSE_CONSECUTIVE_STORE)
# define INTERNAL_TRANSPOSE_INDEX_STORE(I, J, LD) (I * LD + J)
# define INTERNAL_TRANSPOSE_INDEX_LOAD(I, J, LD) (J * LD + I)
#else /* consecutive load and strided store */
# define INTERNAL_TRANSPOSE_INDEX_STORE(I, J, LD) (J * LD + I)
# define INTERNAL_TRANSPOSE_INDEX_LOAD(I, J, LD) (I * LD + J)
#endif

#define INTERNAL_TRANSPOSE_OOP(TYPE, OUT, IN, M0, M1, N0, N1, N, LD, LDO) { \
  const TYPE *const a = (const TYPE*)IN; \
  TYPE *const b = (TYPE*)OUT; \
  libxsmm_blasint i, j; \
  if (LIBXSMM_TRANSPOSE_CHUNK == N && 0 == LIBXSMM_MOD2((uintptr_t)b, LIBXSMM_STATIC_SIMD_WIDTH)) { \
    for (i = M0; i < M1; ++i) { \
      LIBXSMM_PRAGMA_VALIGNED_VARS(b) \
      LIBXSMM_PRAGMA_NONTEMPORAL \
      for (j = N0; j < N0 + LIBXSMM_TRANSPOSE_CHUNK; ++j) { \
        b[INTERNAL_TRANSPOSE_INDEX_STORE(i,j,LDO)] = a[INTERNAL_TRANSPOSE_INDEX_LOAD(i,j,LD)]; \
      } \
    } \
  } \
  else { \
    for (i = M0; i < M1; ++i) { \
      LIBXSMM_PRAGMA_NONTEMPORAL \
      for (j = N0; j < N1; ++j) { \
        b[INTERNAL_TRANSPOSE_INDEX_STORE(i,j,LDO)] = a[INTERNAL_TRANSPOSE_INDEX_LOAD(i,j,LD)]; \
      } \
    } \
  } \
}


/* Based on cache-oblivious scheme as published by Frigo et.al. Further optimization for compile-time bounded loops. */
LIBXSMM_INLINE LIBXSMM_RETARGETABLE void internal_transpose_oop(void *LIBXSMM_RESTRICT out, const void *LIBXSMM_RESTRICT in,
  unsigned int typesize, libxsmm_blasint m0, libxsmm_blasint m1, libxsmm_blasint n0, libxsmm_blasint n1,
  libxsmm_blasint ld, libxsmm_blasint ldo)
{
  const libxsmm_blasint m = m1 - m0, n = n1 - n0;

  if (m * n * typesize <= (LIBXSMM_TRANSPOSE_CACHESIZE / 2)) {
    switch(typesize) {
      case 1: {
        INTERNAL_TRANSPOSE_OOP(char, out, in, m0, m1, n0, n1, n, ld, ldo);
      } break;
      case 2: {
        INTERNAL_TRANSPOSE_OOP(short, out, in, m0, m1, n0, n1, n, ld, ldo);
      } break;
      case 4: {
        INTERNAL_TRANSPOSE_OOP(float, out, in, m0, m1, n0, n1, n, ld, ldo);
      } break;
      case 8: {
        INTERNAL_TRANSPOSE_OOP(double, out, in, m0, m1, n0, n1, n, ld, ldo);
      } break;
      case 16: {
        typedef struct dvec2_t { double value[2]; } dvec2_t;
        INTERNAL_TRANSPOSE_OOP(dvec2_t, out, in, m0, m1, n0, n1, n, ld, ldo);
      } break;
      default: {
#if !defined(NDEBUG) /* library code is expected to be mute */
        fprintf(stderr, "LIBXSMM: unsupported element type in transpose!\n");
#endif
        assert(0);
      }
    }
  }
  else if (m >= n) {
    const libxsmm_blasint mi = (m0 + m1) / 2;
    internal_transpose_oop(out, in, typesize, m0, mi, n0, n1, ld, ldo);
    internal_transpose_oop(out, in, typesize, mi, m1, n0, n1, ld, ldo);
  }
  else {
#if (0 < LIBXSMM_TRANSPOSE_CHUNK)
    if (LIBXSMM_TRANSPOSE_CHUNK < n) {
      const libxsmm_blasint ni = n0 + LIBXSMM_TRANSPOSE_CHUNK;
      internal_transpose_oop(out, in, typesize, m0, m1, n0, ni, ld, ldo);
      internal_transpose_oop(out, in, typesize, m0, m1, ni, n1, ld, ldo);
    }
    else
#endif
    {
      const libxsmm_blasint ni = (n0 + n1) / 2;
      internal_transpose_oop(out, in, typesize, m0, m1, n0, ni, ld, ldo);
      internal_transpose_oop(out, in, typesize, m0, m1, ni, n1, ld, ldo);
    }
  }
}


LIBXSMM_API_DEFINITION void libxsmm_transpose_oop(void* out, const void* in, unsigned int typesize,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
#if !defined(NDEBUG) /* library code is expected to be mute */
  if (ld < m && ldo < n) {
    fprintf(stderr, "LIBXSMM: the leading dimensions of the transpose are too small!\n");
  }
  else if (ld < m) {
    fprintf(stderr, "LIBXSMM: the leading dimension of the transpose input is too small!\n");
  }
  else if (ldo < n) {
    fprintf(stderr, "LIBXSMM: the leading dimension of the transpose output is too small!\n");
  }
#endif
#if defined(__MKL) || defined(MKL_DIRECT_CALL_SEQ) || defined(MKL_DIRECT_CALL)
  if (8 == typesize) {
    mkl_domatcopy('C', 'T', m, n, 1.0, (const double*)in, ld, (double*)out, ldo);
  }
  else if (4 == typesize) {
    mkl_somatcopy('C', 'T', m, n, 1.f, (const float*)in, ld, (float*)out, ldo);
  }
  else if (16 == typesize) {
    const MKL_Complex16 one = { 1.0/*real*/, 0.0/*imag*/ };
    mkl_zomatcopy('C', 'T', m, n, one, (const MKL_Complex16*)in, ld, (MKL_Complex16*)out, ldo);
  }
  else
#endif
  {
    internal_transpose_oop(out, in, typesize, 0, m, 0, n, ld, ldo);
  }
}


LIBXSMM_API_DEFINITION void libxsmm_transpose_inp(void* inout, unsigned int typesize,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld)
{
  LIBXSMM_UNUSED(inout); LIBXSMM_UNUSED(typesize); LIBXSMM_UNUSED(m); LIBXSMM_UNUSED(n); LIBXSMM_UNUSED(ld);
  assert(0/*Not yet implemented!*/);
}


#if defined(LIBXSMM_BUILD)

LIBXSMM_API_DEFINITION void libxsmm_stranspose_oop(float* out, const float* in,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  libxsmm_transpose_oop(out, in, sizeof(float), m, n, ld, ldo);
}


LIBXSMM_API_DEFINITION void libxsmm_dtranspose_oop(double* out, const double* in,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  libxsmm_transpose_oop(out, in, sizeof(double), m, n, ld, ldo);
}


LIBXSMM_API_DEFINITION void libxsmm_stranspose_inp(float* inout,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld)
{
  libxsmm_transpose_inp(inout, sizeof(float), m, n, ld);
}


LIBXSMM_API_DEFINITION void libxsmm_dtranspose_inp(double* inout,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld)
{
  libxsmm_transpose_inp(inout, sizeof(double), m, n, ld);
}

#endif /*defined(LIBXSMM_BUILD)*/

