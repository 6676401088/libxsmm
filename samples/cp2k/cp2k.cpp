/******************************************************************************
** Copyright (c) 2013-2015, Intel Corporation                                **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
******************************************************************************/
/* Hans Pabst (Intel Corp.)
******************************************************************************/
#include <libxsmm.h>
#include <libxsmm_timer.h>

#if defined(LIBXSMM_OFFLOAD_BUILD)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif

#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cmath>

#if defined(__MKL) || defined(MKL_DIRECT_CALL_SEQ) || defined(MKL_DIRECT_CALL)
# include <mkl_service.h>
#endif

#if defined(_OPENMP)
# include <omp.h>
#endif

#if defined(LIBXSMM_OFFLOAD_BUILD)
# pragma offload_attribute(pop)
#endif

// aheineck: @TODO This line is broken -> 10% less performance, going back to static buffer size
//#define CP2K_MAX_SIZE (LIBXSMM_MAX_MNK / LIBXSMM_MAX(LIBXSMM_MAX(LIBXSMM_AVG_M, LIBXSMM_AVG_N), LIBXSMM_AVG_K))
#define CP2K_MAX_SIZE (80 * 80)
/** >1: number of locks, =1: omp critical, =0: atomic */
#define CP2K_SYNCHRONIZATION 0
// ensures sufficient parallel slack
#define CP2K_MIN_NPARALLEL 240
// ensures amortized atomic overhead
#define CP2K_MIN_NLOCAL 160
// OpenMP schedule policy (and chunk size)
#if defined(__MIC__)
# define CP2K_SCHEDULE schedule(static,1)
#else
# define CP2K_SCHEDULE
#endif
// enable result validation
#define CP2K_CHECK


#if defined(_OPENMP) && defined(CP2K_SYNCHRONIZATION) && (1 < (CP2K_SYNCHRONIZATION))
LIBXSMM_RETARGETABLE class LIBXSMM_RETARGETABLE lock_type {
public:
  lock_type() {
    for (int i = 0; i < (CP2K_SYNCHRONIZATION); ++i) omp_init_lock(m_lock + i);
  }
  ~lock_type() {
    for (int i = 0; i < (CP2K_SYNCHRONIZATION); ++i) omp_destroy_lock(m_lock + i);
  }
public:
  void acquire(const void* address) {
    omp_set_lock(m_lock + LIBXSMM_HASH2(address, LIBXSMM_ALIGNMENT, CP2K_SYNCHRONIZATION));
  }
  void release(const void* address) {
    omp_unset_lock(m_lock + LIBXSMM_HASH2(address, LIBXSMM_ALIGNMENT, CP2K_SYNCHRONIZATION));
  }
private:
  omp_lock_t m_lock[CP2K_SYNCHRONIZATION];
} lock;
#endif


template<int Seed>
struct LIBXSMM_RETARGETABLE init {
  template<typename T> init(T *LIBXSMM_RESTRICT dst, int nrows, int ncols, int n = 0, int ld = 0) {
    const int ldx = 0 == ld ? ncols : ld;
    const int minval = n + Seed, addval = (nrows - 1) * ldx + (ncols - 1);
    const int maxval = std::max(std::abs(minval), addval);
    const double norm = 0 != maxval ? (1.0 / maxval) : 1.0;
    for (int i = 0; i < nrows; ++i) {
      for (int j = 0; j < ncols; ++j) {
        const double value = static_cast<double>(i * ldx + j + minval);
        dst[i*ldx+j] = static_cast<T>(norm * (value - 0.5 * addval));
      }
    }
  }
};


template<typename T>
LIBXSMM_RETARGETABLE void add(T *LIBXSMM_RESTRICT dst, const T *LIBXSMM_RESTRICT src, int nrows, int ncols, int ld_src = 0)
{
  const int ld = 0 == ld_src ? ncols : ld_src;
#if defined(_OPENMP) && defined(CP2K_SYNCHRONIZATION) && (0 < (CP2K_SYNCHRONIZATION))
# if (1 == (CP2K_SYNCHRONIZATION))
# pragma omp critical(smmadd)
# else
  lock.acquire(dst);
# endif
#endif
  {
    for (int i = 0; i < nrows; ++i) {
      LIBXSMM_PRAGMA_LOOP_COUNT(1, LIBXSMM_LD(LIBXSMM_MAX_M, LIBXSMM_MAX_N), LIBXSMM_LD(LIBXSMM_AVG_M, LIBXSMM_AVG_N))
      for (int j = 0; j < ncols; ++j) {
        const T value = src[i*ld+j];
#if defined(_OPENMP) && (!defined(CP2K_SYNCHRONIZATION) || (0 == (CP2K_SYNCHRONIZATION)))
#       pragma omp atomic
#endif
        dst[i*ncols+j] += value;
      }
    }
  }
#if defined(_OPENMP) && defined(CP2K_SYNCHRONIZATION) && (1 < (CP2K_SYNCHRONIZATION))
  lock.release(dst);
#endif
}


template<typename T>
LIBXSMM_RETARGETABLE double max_diff(const T *LIBXSMM_RESTRICT result, const T *LIBXSMM_RESTRICT expect, int nrows, int ncols, int ld = 0)
{
  const int ldx = 0 == ld ? ncols : ld;
  double diff = 0;
  for (int i = 0; i < nrows; ++i) {
    for (int j = 0; j < ncols; ++j) {
      const int k = i * ldx + j;
      diff = std::max(diff, std::abs(static_cast<double>(result[k]) - static_cast<double>(expect[k])));
    }
  }
  return diff;
}


int main(int argc, char* argv[])
{
  try {
    typedef double T;
    const int m = 1 < argc ? std::atoi(argv[1]) : 23;
    const int q = ((1ULL << 30) / (3 * m * m * sizeof(T)));
    const int r = 2 < argc ? (0 < std::atoi(argv[2]) ? std::atoi(argv[2]) : ('+' == *argv[2]
      ? (q << std::strlen(argv[2])) : ('-' == *argv[2]
      ? (q >> std::strlen(argv[2])) : 0))) : 0;
    const int t = 3 < argc ? (0 < std::atoi(argv[3]) ? std::atoi(argv[3]) : ('+' == *argv[3]
      ? ((CP2K_MIN_NLOCAL) << std::strlen(argv[3])) : ('-' == *argv[3]
      ? ((CP2K_MIN_NLOCAL) >> std::strlen(argv[3])) : -1))) : -1;
    const int n = 4 < argc ? std::atoi(argv[4]) : m;
    const int k = 5 < argc ? std::atoi(argv[5]) : m;

    const int ldc = 0 == (LIBXSMM_GEMM_FLAG_ALIGN_C & LIBXSMM_FLAGS) ? LIBXSMM_LD(m, n) : LIBXSMM_ALIGN_VALUE(LIBXSMM_LD(m, n), sizeof(T), LIBXSMM_ALIGNMENT);
    const int ldcsize = ldc * LIBXSMM_LD(n, m);
    if ((CP2K_MAX_SIZE) < csize) {
      throw std::runtime_error("The size M x N is exceeding CP2K_MAX_SIZE!");
    }

    const int asize = m * k, bsize = k * n, aspace = LIBXSMM_ALIGNMENT / sizeof(T);
    const int s = 0 < r ? r : ((2ULL << 30) / ((asize + bsize) * sizeof(T))); // 2 GByte
    const int u = 0 < t ? t : static_cast<int>(std::sqrt(static_cast<double>(s) * CP2K_MIN_NLOCAL / CP2K_MIN_NPARALLEL) + 0.5);
    const size_t bwsize = (s * (asize + bsize)/*load*/ + ((s + u - 1) / u) * csize * 2/*accumulate*/) * sizeof(T);
    const double gflops = 2.0 * s * m * n * k * 1E-9;

    LIBXSMM_RETARGETABLE struct LIBXSMM_RETARGETABLE raii { // avoid std::vector (first-touch init. causes NUMA issue)
      T *a, *b, *c;
      raii(int asize, int bsize, int csize): a(new T[asize]), b(new T[bsize]), c(new T[csize]) {}
      ~raii() { delete[] a; delete[] b; delete[] c; }
    } buffer(s * asize + aspace - 1, s * bsize + aspace - 1, csize);
    T *const a = LIBXSMM_ALIGN(buffer.a, LIBXSMM_ALIGNMENT);
    T *const b = LIBXSMM_ALIGN(buffer.b, LIBXSMM_ALIGNMENT);
    T * /*const*/ c = buffer.c; // no alignment, but thread-local array will be aligned

#if defined(_OPENMP)
#   pragma omp parallel for CP2K_SCHEDULE
#endif
    for (int i = 0; i < s; ++i) {
      init<42>(a + i * asize, m, k, i);
      init<24>(b + i * bsize, k, n, i);
    }

#if defined(LIBXSMM_OFFLOAD_BUILD)
#   pragma offload target(LIBXSMM_OFFLOAD_TARGET) in(a: length(s * asize)) in(b: length(s * bsize)) out(c: length(csize))
#endif
    {
#if defined(MKL_ENABLE_AVX512_MIC)
      mkl_enable_instructions(MKL_ENABLE_AVX512_MIC);
#endif
      // initialize LIBXSMM
      libxsmm_init();

      fprintf(stdout, "m=%i n=%i k=%i ldc=%i (%s) size=%i batch=%i memory=%.f MB\n\n",
        m, n, k, ldc, 0 != LIBXSMM_ROW_MAJOR ? "row-major" : "column-major", s, u,
        1.0 * (s * (asize + bsize) * sizeof(T)) / (1 << 20));

#if defined(CP2K_CHECK)
      LIBXSMM_RETARGETABLE struct LIBXSMM_RETARGETABLE raii { // avoid std::vector (first-touch init. causes NUMA issue)
        T *expect;
        explicit raii(int size): expect(new T[size]) {}
        ~raii() { delete[] expect; }
      } expect_buffer(csize);
      T *const expect = expect_buffer.expect;
      std::fill_n(expect, csize, 0);
#else
      T *const expect = c;
#endif

      { // LAPACK/BLAS3 (warmup BLAS Library)
#if defined(_OPENMP)
#       pragma omp parallel for CP2K_SCHEDULE
#endif
        for (int i = 0; i < s; i += u) {
          LIBXSMM_ALIGNED(T tmp[CP2K_MAX_SIZE], LIBXSMM_ALIGNMENT);
          for (int j = 0; j < (CP2K_MAX_SIZE); ++j) tmp[j] = 0; // clear
          for (int j = 0; j < LIBXSMM_MIN(u, s - i); ++j) {
            libxsmm_blasmm(LIBXSMM_FLAGS, m, n, k, a + (i + j) * asize, b + (i + j) * bsize, tmp);
          }
          add(expect, tmp, m, n, ldc); // atomic
        }
      }

      { // LAPACK/BLAS3 (reference)
        fprintf(stdout, "LAPACK/BLAS...\n");
        std::fill_n(expect, csize, 0);
        const unsigned long long start = libxsmm_timer_tick();
#if defined(_OPENMP)
#       pragma omp parallel for CP2K_SCHEDULE
#endif
        for (int i = 0; i < s; i += u) {
          LIBXSMM_ALIGNED(T tmp[CP2K_MAX_SIZE], LIBXSMM_ALIGNMENT);
          for (int j = 0; j < (CP2K_MAX_SIZE); ++j) tmp[j] = 0; // clear
          for (int j = 0; j < LIBXSMM_MIN(u, s - i); ++j) {
            libxsmm_blasmm(LIBXSMM_FLAGS, m, n, k, a + (i + j) * asize, b + (i + j) * bsize, tmp);
          }
          add(expect, tmp, m, n, ldc); // atomic
        }
        const double duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
        if (0 < duration) {
          fprintf(stdout, "\tperformance: %.1f GFLOPS/s\n", gflops / duration);
          fprintf(stdout, "\tbandwidth: %.1f GB/s\n", bwsize / (duration * (1 << 30)));
          fprintf(stdout, "\tcalls/s: %.0f Hz\n", s / duration);
        }
        fprintf(stdout, "\tduration: %.0f ms\n", 1000.0 * duration);
      }

      { // inline an optimized implementation
        fprintf(stdout, "Inlined...\n");
        std::fill_n(c, csize, 0);
        const unsigned long long start = libxsmm_timer_tick();
#if defined(_OPENMP)
#       pragma omp parallel for CP2K_SCHEDULE
#endif
        for (int i = 0; i < s; i += u) {
          LIBXSMM_ALIGNED(T tmp[CP2K_MAX_SIZE], LIBXSMM_ALIGNMENT);
          const T *ai = a + i * asize, *bi = b + i * bsize;
          for (int j = 0; j < (CP2K_MAX_SIZE); ++j) tmp[j] = 0; // clear
          for (int j = 0; j < LIBXSMM_MIN(u, s - i); ++j) {
            const T *const aij = ai + asize, *const bij = bi + bsize;
            LIBXSMM_XIMM(LIBXSMM_FLAGS, m, n, k, ai, bi, tmp, 0, 0);
            ai = aij;
            bi = bij;
          }
          add(c, tmp, m, n, ldc); // atomic
        }
        const double duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
        if (0 < duration) {
          fprintf(stdout, "\tperformance: %.1f GFLOPS/s\n", gflops / duration);
          fprintf(stdout, "\tbandwidth: %.1f GB/s\n", bwsize / (duration * (1 << 30)));
          fprintf(stdout, "\tcalls/s: %.0f Hz\n", s / duration);
        }
        fprintf(stdout, "\tduration: %.0f ms\n", 1000.0 * duration);
#if defined(CP2K_CHECK)
        fprintf(stdout, "\tdiff=%f\n", max_diff(c, expect, m, n));
#endif
      }

      { // auto-dispatched
        fprintf(stdout, "Dispatched...\n");
        std::fill_n(c, csize, 0);
        const unsigned long long start = libxsmm_timer_tick();
#if defined(_OPENMP)
#       pragma omp parallel for CP2K_SCHEDULE
#endif
        for (int i = 0; i < s; i += u) {
          LIBXSMM_ALIGNED(T tmp[CP2K_MAX_SIZE], LIBXSMM_ALIGNMENT);
          const T *ai = a + i * asize, *bi = b + i * bsize;
          for (int j = 0; j < (CP2K_MAX_SIZE); ++j) tmp[j] = 0; // clear
          for (int j = 0; j < LIBXSMM_MIN(u, s - i); ++j) {
#if 0
            printf("outer-iteration: %i, inner-iterations: %i, c-address: %lu, 64byte-alginment: %lu, 32byte-alignment: %lu, 16byte-alignment: %lu,\n", i, LIBXSMM_MIN(u, s - i), (size_t)tmp, ((size_t)tmp)%64, ((size_t)tmp)%32, ((size_t)tmp)%16);
#endif
            const T *const aij = ai + asize, *const bij = bi + bsize;
#if (0 != LIBXSMM_PREFETCH)
            libxsmm_mm(LIBXSMM_FLAGS, m, n, k, ai, bi, tmp, aij + asize, bij + bsize, tmp);
#else
            libxsmm_mm(LIBXSMM_FLAGS, m, n, k, ai, bi, tmp);
#endif
            ai = aij;
            bi = bij;
          }
          add(c, tmp, m, n, ldc); // atomic
        }
        const double duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
        if (0 < duration) {
          fprintf(stdout, "\tperformance: %.1f GFLOPS/s\n", gflops / duration);
          fprintf(stdout, "\tbandwidth: %.1f GB/s\n", bwsize / (duration * (1 << 30)));
          fprintf(stdout, "\tcalls/s: %.0f Hz\n", s / duration);
        }
        fprintf(stdout, "\tduration: %.0f ms\n", 1000.0 * duration);
#if defined(CP2K_CHECK)
        fprintf(stdout, "\tdiff=%f\n", max_diff(c, expect, m, n));
#endif
      }

      const libxsmm_function<T> xmm(m, n, k);
      if (xmm) { // specialized routine
        fprintf(stdout, "Specialized...\n");
        std::fill_n(c, csize, 0);
        const unsigned long long start = libxsmm_timer_tick();
#if defined(_OPENMP)
#       pragma omp parallel for CP2K_SCHEDULE
#endif
        for (int i = 0; i < s; i += u) {
          LIBXSMM_ALIGNED(T tmp[CP2K_MAX_SIZE], LIBXSMM_ALIGNMENT);
          const T *ai = a + i * asize, *bi = b + i * bsize;
          for (int j = 0; j < (CP2K_MAX_SIZE); ++j) tmp[j] = 0; // clear
          for (int j = 0; j < LIBXSMM_MIN(u, s - i); ++j) {
            const T *const aij = ai + asize, *const bij = bi + bsize;
#if (0 != LIBXSMM_PREFETCH)
            xmm(ai, bi, tmp, aij + asize, bij + bsize, tmp);
#else
            xmm(ai, bi, tmp);
#endif
            ai = aij;
            bi = bij;
          }
          add(c, tmp, m, n, ldc); // atomic
        }
        const double duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
        if (0 < duration) {
          fprintf(stdout, "\tperformance: %.1f GFLOPS/s\n", gflops / duration);
          fprintf(stdout, "\tbandwidth: %.1f GB/s\n", bwsize / (duration * (1 << 30)));
          fprintf(stdout, "\tcalls/s: %.0f Hz\n", s / duration);
        }
        fprintf(stdout, "\tduration: %.0f ms\n", 1000.0 * duration);
#if defined(CP2K_CHECK)
        fprintf(stdout, "\tdiff=%f\n", max_diff(c, expect, m, n));
#endif
      }

      // finalize LIBXSMM
      libxsmm_finalize();
      fprintf(stdout, "Finished\n");
    }
  }
  catch(const std::exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return EXIT_FAILURE;
  }
  catch(...) {
    fprintf(stderr, "Error: unknown exception caught!\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
