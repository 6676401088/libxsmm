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
#include "libxsmm_main.h"
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


LIBXSMM_API_DEFINITION void libxsmm_trans_init(int archid)
{
  libxsmm_trans_chunksize = LIBXSMM_TRANS_MAX_CHUNKSIZE;
#if defined(__MIC__)
  LIBXSMM_UNUSED(archid);
#else
  if (LIBXSMM_X86_AVX512_MIC == archid)
#endif
  {
    libxsmm_trans_chunksize = LIBXSMM_TRANS_MIN_CHUNKSIZE;
  }
}


LIBXSMM_API_DEFINITION void libxsmm_trans_finalize(void)
{
}


LIBXSMM_INLINE LIBXSMM_RETARGETABLE void internal_otrans(void *LIBXSMM_RESTRICT out, const void *LIBXSMM_RESTRICT in,
  unsigned int typesize, libxsmm_blasint m0, libxsmm_blasint m1, libxsmm_blasint n0, libxsmm_blasint n1,
  libxsmm_blasint ld, libxsmm_blasint ldo)
{
  LIBXSMM_OTRANS_MAIN(LIBXSMM_NOOP, internal_otrans, out, in, typesize, m0, m1, n0, n1, ld, ldo);
}


LIBXSMM_API_DEFINITION int libxsmm_otrans(void* out, const void* in, unsigned int typesize,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  int result = EXIT_SUCCESS;
#if !defined(NDEBUG) /* library code is expected to be mute */
  static LIBXSMM_TLS int trans_error = 0;
#endif
  LIBXSMM_INIT

  if (ld >= m && ldo >= n) {
    if (out != in) {
      internal_otrans(out, in, typesize, 0, m, 0, n, ld, ldo);
    }
    else if (ld == ldo) {
      libxsmm_itrans(out, typesize, m, n, ld);
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


LIBXSMM_API_DEFINITION int libxsmm_itrans(void* inout, unsigned int typesize,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld)
{
  LIBXSMM_UNUSED(inout); LIBXSMM_UNUSED(typesize); LIBXSMM_UNUSED(m); LIBXSMM_UNUSED(n); LIBXSMM_UNUSED(ld);
  assert(0/*TODO: not yet implemented!*/);
  LIBXSMM_INIT
  return EXIT_FAILURE;
}


#if defined(LIBXSMM_BUILD)

LIBXSMM_API void libxsmmf_otrans(void*, const void*, unsigned int, libxsmm_blasint, libxsmm_blasint, libxsmm_blasint, libxsmm_blasint);
LIBXSMM_API_DEFINITION void libxsmmf_otrans(void* out, const void* in, unsigned int typesize,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  libxsmmf_otrans(out, in, typesize, m, n, ld, ldo);
}


/** code variant for the Fortran interface, which does not produce a return value */
LIBXSMM_API void libxsmmf_sotrans(float*, const float*, libxsmm_blasint, libxsmm_blasint, libxsmm_blasint, libxsmm_blasint);
LIBXSMM_API_DEFINITION void libxsmmf_sotrans(float* out, const float* in,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  libxsmmf_otrans(out, in, sizeof(float), m, n, ld, ldo);
}


LIBXSMM_API_DEFINITION int libxsmm_sotrans(float* out, const float* in,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  return libxsmm_otrans(out, in, sizeof(float), m, n, ld, ldo);
}


/** code variant for the Fortran interface, which does not produce a return value */
LIBXSMM_API void libxsmmf_dotrans(double*, const double*, libxsmm_blasint, libxsmm_blasint, libxsmm_blasint, libxsmm_blasint);
LIBXSMM_API_DEFINITION void libxsmmf_dotrans(double* out, const double* in,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  libxsmmf_otrans(out, in, sizeof(double), m, n, ld, ldo);
}


LIBXSMM_API_DEFINITION int libxsmm_dotrans(double* out, const double* in,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld, libxsmm_blasint ldo)
{
  return libxsmm_otrans(out, in, sizeof(double), m, n, ld, ldo);
}


LIBXSMM_API_DEFINITION int libxsmm_sitrans(float* inout,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld)
{
  return libxsmm_itrans(inout, sizeof(float), m, n, ld);
}


LIBXSMM_API_DEFINITION int libxsmm_ditrans(double* inout,
  libxsmm_blasint m, libxsmm_blasint n, libxsmm_blasint ld)
{
  return libxsmm_itrans(inout, sizeof(double), m, n, ld);
}

#endif /*defined(LIBXSMM_BUILD)*/

