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
/* Christopher Dahnken (Intel Corp.), Hans Pabst (Intel Corp.),
 * Alfio Lazzaro (CRAY Inc.), and Gilles Fourestey (CSCS)
******************************************************************************/
#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <vector>
#include <cmath>

#include <libxsmm.h>

#define USE_AUTODISPATCH
#define USE_CHECK
//#define USE_PRINT


template<typename T> void nrand(T& a)
{
  static const double scale = 1.0 / RAND_MAX;
  a = static_cast<T>(scale * (2 * std::rand() - RAND_MAX));
}


template<typename T> void print(const T* matrix, int nrows, int ncols)
{
  for (int i = 0; i < nrows; ++i) {
    for (int j = 0; j < ncols; ++j) {
#if (0 != LIBXSMM_ROW_MAJOR)
      fprintf(stderr, "%6.2f", matrix[i*ncols+j]);
#else
      fprintf(stderr, "%6.2f", matrix[j*nrows+i]);
#endif
    }
    fprintf(stderr, "\n");
  }
}


int main(int argc, char* argv[])
{
  try {
    typedef double T;
    const int m = 1 < argc ? std::atoi(argv[1]) : 23;
    const int n = 2 < argc ? std::atoi(argv[2]) : m;
    const int k = 3 < argc ? std::atoi(argv[3]) : m;

#if (0 != LIBXSMM_ALIGNED_STORES)
# if (0 != LIBXSMM_ROW_MAJOR)
    const int ldc = LIBXSMM_ALIGN_VALUE(int, T, n, LIBXSMM_ALIGNED_STORES);
    const int csize = m * ldc + (LIBXSMM_ALIGNED_STORES) / sizeof(T) - 1;
# else
    const int ldc = LIBXSMM_ALIGN_VALUE(int, T, m, LIBXSMM_ALIGNED_STORES);
    const int csize = n * ldc + (LIBXSMM_ALIGNED_STORES) / sizeof(T) - 1;
# endif
#else
# if (0 != LIBXSMM_ROW_MAJOR)
    const int ldc = n, csize = m * ldc;
# else
    const int ldc = m, csize = n * ldc;
# endif
#endif

    const int asize = m * k, bsize = k * n;
    std::vector<T> va(asize), vb(bsize);
    std::for_each(va.begin(), va.end(), nrand<T>);
    std::for_each(vb.begin(), vb.end(), nrand<T>);
    const T *const a = &va[0], *const b = &vb[0];

    std::vector<T> vresult(csize, 0.0), vexpect(csize, 0.0);
    T *const result = LIBXSMM_ALIGN(T*, &vresult[0], LIBXSMM_ALIGNED_STORES);
    T *const expect = LIBXSMM_ALIGN(T*, &vexpect[0], LIBXSMM_ALIGNED_STORES);
#if (0 != LIBXSMM_ALIGNED_STORES)
    assert(0 == (reinterpret_cast<uintptr_t>(result) % (LIBXSMM_ALIGNED_STORES)));
    assert(0 == (reinterpret_cast<uintptr_t>(expect) % (LIBXSMM_ALIGNED_STORES)));
#endif

    fprintf(stderr, "m=%i n=%i k=%i ldc=%i\n", m, n, k, ldc);
    const libxsmm_mm_dispatch<T> xmm(m, n, k);
    if (xmm) {
      fprintf(stderr, "using a specialized routine\n");
    }

#if defined(USE_PRINT)
    fprintf(stderr, "a =\n");
    print(a, m, k);
    fprintf(stderr, "b =\n");
    print(b, k, n);
#endif

#if defined(USE_AUTODISPATCH)
    libxsmm_mm(m, n, k, a, b, result);
#else // custom dispatch
    if (xmm) {
      // specialized routine
      xmm(a, b, result);
    }
    else if (LIBXSMM_MAX_MNK >= (m * n * k)) {
      // inline an optimized implementation
      libxsmm_imm(m, n, k, a, b, result);
    }
    else { // LAPACK/BLAS3 (fallback)
      libxsmm_blasmm(m, n, k, a, b, result);
    }
#endif

#if defined(USE_PRINT)
    fprintf(stderr, "result =\n");
    print(result, m, n);
#endif

#if defined(USE_CHECK)
    // check against a different implementation
    if (LIBXSMM_MAX_MNK >= (m * n * k)) {
      libxsmm_blasmm(m, n, k, a, b, expect);
    }
    else {
      libxsmm_imm(m, n, k, a, b, expect);
    }
# if defined(USE_PRINT)
    fprintf(stderr, "expect =\n");
    print(expect, m, n);
# endif
    double diff = 0;
    for (int i = 0; i < m; ++i) {
      for (int j = 0; j < n; ++j) {
#if (0 != LIBXSMM_ROW_MAJOR)
        const int k = i * ldc + j;
#else
        const int k = j * ldc + i;
#endif
        diff = std::max(diff, std::abs(static_cast<double>(result[k]) - static_cast<double>(expect[k])));
      }
    }
    fprintf(stderr, "diff=%f\n", diff);
#endif
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
