/******************************************************************************
** Copyright (c) 2016, Intel Corporation                                     **
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
#include "libxsmm_trans.h"
#include "libxsmm_ext.h"
#include <libxsmm_sync.h>

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <stdlib.h>
#if !defined(NDEBUG)
# include <stdio.h>
#endif
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif


#if defined(LIBXSMM_EXT_TASKS)
LIBXSMM_INLINE LIBXSMM_RETARGETABLE void internal_ext_otrans(void *LIBXSMM_RESTRICT out, const void *LIBXSMM_RESTRICT in,
  unsigned int typesize, libxsmm_blasint m0, libxsmm_blasint m1, libxsmm_blasint n0, libxsmm_blasint n1,
  libxsmm_blasint ld, libxsmm_blasint ldo)
{
  LIBXSMM_OTRANS_MAIN(LIBXSMM_EXT_TSK_KERNEL, internal_ext_otrans, out, in, typesize, m0, m1, n0, n1, ld, ldo);
}
#endif /*defined(LIBXSMM_EXT_TASKS)*/


LIBXSMM_API_DEFINITION int libxsmm_otrans_omp(void* out, const void* in, unsigned int typesize,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  int result = EXIT_SUCCESS;
#if !defined(NDEBUG) /* library code is expected to be mute */
  static LIBXSMM_TLS int trans_error = 0;
#endif
  LIBXSMM_INIT

  if (ld >= m && ldo >= n) {
    if (out != in) {
#if defined(LIBXSMM_EXT_TASKS)
      if (0 != libxsmm_mt) { /* enable OpenMP support */
        if (0 == LIBXSMM_MOD2(libxsmm_mt, 2)) { /* even: enable internal parallelization */
          LIBXSMM_EXT_TSK_PARALLEL_ONLY
          internal_ext_otrans(out, in, typesize, 0, m, 0, n, ld, ldo);
          /* implicit synchronization (barrier) */
        }
        else { /* odd: prepare for external parallelization */
          LIBXSMM_EXT_SINGLE
          internal_ext_otrans(out, in, typesize, 0, m, 0, n, ld, ldo);
          if (1 == libxsmm_mt) { /* allow to omit synchronization */
            LIBXSMM_EXT_TSK_SYNC
          }
        }
      }
      else
#endif
      {
        libxsmm_otrans(out, in, typesize, m, n, ld, ldo);
      }
    }
    else if (ld == ldo) {
      libxsmm_itrans/*TODO: omp*/(out, typesize, m, n, ld);
    }
    else {
#if !defined(NDEBUG) /* library code is expected to be mute */
      if (0 == trans_error) {
        fprintf(stderr, "LIBXSMM: output location of the transpose must be different from the input!\n");
        trans_error = 1;
      }
#endif
      result = EXIT_FAILURE;
    }
  }
  else {
#if !defined(NDEBUG) /* library code is expected to be mute */
    if (0 == trans_error) {
      if (ld < m && ldo < n) {
        fprintf(stderr, "LIBXSMM: the leading dimensions of the transpose are too small!\n");
      }
      else if (ld < m) {
        fprintf(stderr, "LIBXSMM: the leading dimension of the transpose input is too small!\n");
      }
      else {
        assert(ldo < n);
        fprintf(stderr, "LIBXSMM: the leading dimension of the transpose output is too small!\n");
      }
      trans_error = 1;
    }
#endif
    result = EXIT_FAILURE;
  }

  return result;
}


LIBXSMM_API_DEFINITION int libxsmm_sotrans_omp(float* out, const float* in,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  return libxsmm_otrans_omp(out, in, sizeof(float), m, n, ld, ldo);
}


LIBXSMM_API_DEFINITION int libxsmm_dotrans_omp(double* out, const double* in,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  return libxsmm_otrans_omp(out, in, sizeof(double), m, n, ld, ldo);
}

