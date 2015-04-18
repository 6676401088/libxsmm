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

#if defined(LIBXSMM_OFFLOAD)
# pragma offload_attribute(push,target(mic))
#endif

#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <vector>
#include <cmath>

#if defined(_OPENMP)
# include <omp.h>
#endif

#if defined(LIBXSMM_OFFLOAD)
# pragma offload_attribute(pop)
#endif


template<typename T>
LIBXSMM_TARGET(mic) void nrand(T& a)
{
  static const double scale = 1.0 / RAND_MAX;
  a = static_cast<T>(scale * (2 * std::rand() - RAND_MAX));
}


int main(int argc, char* argv[])
{
  try {
    typedef double T;
    const int s = 100000;
    const int m = 1 < argc ? std::atoi(argv[1]) : 23;
    const int n = 2 < argc ? std::atoi(argv[2]) : m;
    const int k = 3 < argc ? std::atoi(argv[3]) : m;

#if (0 != LIBXSMM_ROW_MAJOR)
# if (0 < LIBXSMM_ALIGNED_STORES)
    const int ldc = LIBXSMM_ALIGN_VALUE(int, T, n, LIBXSMM_ALIGNED_STORES);
# else
    const int ldc = n;
# endif
    const int csize = m * ldc;
#else
# if (0 < LIBXSMM_ALIGNED_STORES)
    const int ldc = LIBXSMM_ALIGN_VALUE(int, T, m, LIBXSMM_ALIGNED_STORES);
# else
    const int ldc = m;
# endif
    const int csize = n * ldc;
#endif

    const int asize = m * k, bsize = k * n, aspace = (LIBXSMM_ALIGNMENT) / sizeof(T);
    std::vector<T> va(s * asize + aspace - 1), vb(s * bsize + aspace - 1), vc(s * csize + aspace - 1);
    std::for_each(va.begin(), va.end(), nrand<T>);
    std::for_each(vb.begin(), vb.end(), nrand<T>);

    const T *const a = LIBXSMM_ALIGN(const T*, &va[0], LIBXSMM_ALIGNMENT);
    const T *const b = LIBXSMM_ALIGN(const T*, &vb[0], LIBXSMM_ALIGNMENT);
    T * /*const*/ c = LIBXSMM_ALIGN(T*, &vc[0], LIBXSMM_ALIGNMENT);

#if defined(LIBXSMM_OFFLOAD)
#   pragma offload target(mic) in(a: length(s * asize)) in(b: length(s * bsize)) out(c: length(s * csize))
#endif
    {
      fprintf(stdout, "m=%i n=%i k=%i ldc=%i (%s) size=%i\n\n", m, n, k, ldc, 0 != (LIBXSMM_ROW_MAJOR) ? "row-major" : "column-major", s);

      { // streaming
        fprintf(stdout, "Streaming...\n");
        std::fill_n(c, s * csize, 0);
#if defined(_OPENMP)
        const double start = omp_get_wtime();
#       pragma omp parallel for
#endif
        for (int i = 0; i < s; ++i) {
          libxsmm_blasmm(m, n, k, a + i * asize, b + i * bsize, c + i * csize);
        }
#if defined(_OPENMP)
        const double duration = omp_get_wtime() - start;
        if (0 < duration) {
          const double gflops = 2.0 * s * m * n * k * 1E-9;
          fprintf(stdout, "\tperformance: %.1f GFLOPS/s\n", gflops / duration);
        }
        fprintf(stdout, "\tduration: %.1f s\n", duration);
#endif
      }

      { // cached
        fprintf(stdout, "Cached...\n");
        std::fill_n(c, csize, 0);
#if defined(_OPENMP)
        const double start = omp_get_wtime();
#       pragma omp parallel for
#endif
        for (int i = 0; i < s; ++i) {
          libxsmm_blasmm(m, n, k, a, b, c);
        }
#if defined(_OPENMP)
        const double duration = omp_get_wtime() - start;
        if (0 < duration) {
          const double gflops = 2.0 * s * m * n * k * 1E-9;
          fprintf(stdout, "\tperformance: %.1f GFLOPS/s\n", gflops / duration);
        }
        fprintf(stdout, "\tduration: %.1f s\n", duration);
#endif
      }

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
