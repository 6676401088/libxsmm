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
#include "libxsmm_ext_gemm.h"
#include "libxsmm_gemm.h"

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#if defined(LIBXSMM_RTLD_NEXT)
# include <dlfcn.h>
#endif
#include <stdlib.h>
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif


LIBXSMM_API_DEFINITION int libxsmm_gemm_init(int archid, int prefetch)
{
  int result = EXIT_SUCCESS;
  union { const void* pv; libxsmm_sgemm_function pf; } fn_sgemm = { NULL };
  union { const void* pv; libxsmm_dgemm_function pf; } fn_dgemm = { NULL };
#if defined(LIBXSMM_RTLD_NEXT)
  fn_sgemm.pv = dlsym(RTLD_NEXT, LIBXSMM_STRINGIFY(LIBXSMM_FSYMBOL(sgemm)));
  fn_dgemm.pv = dlsym(RTLD_NEXT, LIBXSMM_STRINGIFY(LIBXSMM_FSYMBOL(dgemm)));
#endif

  /* internal pre-initialization step */
  libxsmm_gemm_configure(archid, prefetch, fn_sgemm.pf, fn_dgemm.pf);

  { /* behaviour of libxsmm_omp_?gemm routines or LD_PRELOAD ?GEMM routines
     * 0: sequential below-threshold routine (no OpenMP); may fall-back to BLAS,
     * 1: OpenMP-parallelized but without internal parallel region,
     * 2: OpenMP-parallelized with internal parallel region" )
     */
    const char *const env = getenv("LIBXSMM_GEMM");
    if (0 != env && 0 != *env) {
      internal_gemm = atoi(env);
    }
  }

#if defined(LIBXSMM_GEMM_EXTOMP_TASKS)
  { /* consider user input about using (OpenMP-)tasks; this code must be here
    * because maybe only this translation unit is compiled with OpenMP support
    */
    const char *const env_tasks = getenv("LIBXSMM_TASKS");
    if (0 != env_tasks && 0 != *env_tasks) {
      internal_gemm_tasks = atoi(env_tasks);
    }
  }
#endif
#if !defined(__BLAS) || (0 != __BLAS)
  result = (0 != *libxsmm_original_sgemm() && 0 != *libxsmm_original_dgemm()) ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
  assert(EXIT_SUCCESS == result);
  return result;
}


LIBXSMM_API_DEFINITION void libxsmm_gemm_finalize(void)
{
}


LIBXSMM_API_DEFINITION void libxsmm_omp_sgemm(const char* transa, const char* transb,
  const libxsmm_blasint* m, const libxsmm_blasint* n, const libxsmm_blasint* k,
  const float* alpha, const float* a, const libxsmm_blasint* lda,
  const float* b, const libxsmm_blasint* ldb,
  const float* beta, float* c, const libxsmm_blasint* ldc)
{
  const int tm = internal_gemm_tile[1/*SP*/][0/*M*/];
  const int tn = internal_gemm_tile[1/*SP*/][1/*N*/];
  const int tk = internal_gemm_tile[1/*SP*/][2/*K*/];
  const int nt = internal_gemm_nt;
  LIBXSMM_GEMM_DECLARE_FLAGS(flags, transa, transb, m, n, k, a, b, c);
#if !defined(_OPENMP)
  LIBXSMM_UNUSED(nt);
#endif
  if (0 == LIBXSMM_DIV2(internal_gemm, 2)) { /* enable internal parallelization */
    if (0 == internal_gemm_tasks) {
      LIBXSMM_GEMM_EXTOMP_XGEMM(LIBXSMM_GEMM_EXTOMP_FOR_INIT, LIBXSMM_GEMM_EXTOMP_FOR_LOOP_BEGIN_PARALLEL,
        LIBXSMM_GEMM_EXTOMP_FOR_LOOP_BODY, LIBXSMM_GEMM_EXTOMP_FOR_LOOP_END,
        float, flags | LIBXSMM_GEMM_FLAG_F32PREC, nt, tm, tn, tk, *m, *n, *k,
        0 != alpha ? *alpha : ((float)LIBXSMM_ALPHA),
        a, *(lda ? lda : LIBXSMM_LD(m, k)), b, *(ldb ? ldb : LIBXSMM_LD(k, n)),
        0 != beta ? *beta : ((float)LIBXSMM_BETA),
        c, *(ldc ? ldc : LIBXSMM_LD(m, n)));
    }
    else {
      LIBXSMM_GEMM_EXTOMP_XGEMM(LIBXSMM_GEMM_EXTOMP_TSK_INIT, LIBXSMM_GEMM_EXTOMP_TSK_LOOP_BEGIN_PARALLEL,
        LIBXSMM_GEMM_EXTOMP_TSK_LOOP_BODY, LIBXSMM_GEMM_EXTOMP_TSK_LOOP_END,
        float, flags | LIBXSMM_GEMM_FLAG_F32PREC, nt, tm, tn, tk, *m, *n, *k,
        0 != alpha ? *alpha : ((float)LIBXSMM_ALPHA),
        a, *(lda ? lda : LIBXSMM_LD(m, k)), b, *(ldb ? ldb : LIBXSMM_LD(k, n)),
        0 != beta ? *beta : ((float)LIBXSMM_BETA),
        c, *(ldc ? ldc : LIBXSMM_LD(m, n)));
    }
  }
  else { /* potentially sequential or externally parallelized */
    if (0 == internal_gemm_tasks) {
      LIBXSMM_GEMM_EXTOMP_XGEMM(LIBXSMM_GEMM_EXTOMP_FOR_INIT, LIBXSMM_GEMM_EXTOMP_FOR_LOOP_BEGIN,
        LIBXSMM_GEMM_EXTOMP_FOR_LOOP_BODY, LIBXSMM_GEMM_EXTOMP_FOR_LOOP_END,
        float, flags | LIBXSMM_GEMM_FLAG_F32PREC, nt, tm, tn, tk, *m, *n, *k,
        0 != alpha ? *alpha : ((float)LIBXSMM_ALPHA),
        a, *(lda ? lda : LIBXSMM_LD(m, k)), b, *(ldb ? ldb : LIBXSMM_LD(k, n)),
        0 != beta ? *beta : ((float)LIBXSMM_BETA),
        c, *(ldc ? ldc : LIBXSMM_LD(m, n)));
    }
    else {
      LIBXSMM_GEMM_EXTOMP_XGEMM(LIBXSMM_GEMM_EXTOMP_TSK_INIT, LIBXSMM_GEMM_EXTOMP_TSK_LOOP_BEGIN,
        LIBXSMM_GEMM_EXTOMP_TSK_LOOP_BODY, LIBXSMM_GEMM_EXTOMP_TSK_LOOP_END,
        float, flags | LIBXSMM_GEMM_FLAG_F32PREC, nt, tm, tn, tk, *m, *n, *k,
        0 != alpha ? *alpha : ((float)LIBXSMM_ALPHA),
        a, *(lda ? lda : LIBXSMM_LD(m, k)), b, *(ldb ? ldb : LIBXSMM_LD(k, n)),
        0 != beta ? *beta : ((float)LIBXSMM_BETA),
        c, *(ldc ? ldc : LIBXSMM_LD(m, n)));
    }
  }
}


LIBXSMM_API_DEFINITION void libxsmm_omp_dgemm(const char* transa, const char* transb,
  const libxsmm_blasint* m, const libxsmm_blasint* n, const libxsmm_blasint* k,
  const double* alpha, const double* a, const libxsmm_blasint* lda,
  const double* b, const libxsmm_blasint* ldb,
  const double* beta, double* c, const libxsmm_blasint* ldc)
{
  const int tm = internal_gemm_tile[0/*DP*/][0/*M*/];
  const int tn = internal_gemm_tile[0/*DP*/][1/*N*/];
  const int tk = internal_gemm_tile[0/*DP*/][2/*K*/];
  const int nt = internal_gemm_nt;
  LIBXSMM_GEMM_DECLARE_FLAGS(flags, transa, transb, m, n, k, a, b, c);
#if !defined(_OPENMP)
  LIBXSMM_UNUSED(nt);
#endif
  if (0 == LIBXSMM_DIV2(internal_gemm, 2)) { /* enable internal parallelization */
    if (0 == internal_gemm_tasks) {
      LIBXSMM_GEMM_EXTOMP_XGEMM(LIBXSMM_GEMM_EXTOMP_FOR_INIT, LIBXSMM_GEMM_EXTOMP_FOR_LOOP_BEGIN_PARALLEL,
        LIBXSMM_GEMM_EXTOMP_FOR_LOOP_BODY, LIBXSMM_GEMM_EXTOMP_FOR_LOOP_END,
        double, flags, nt, tm, tn, tk, *m, *n, *k,
        0 != alpha ? *alpha : ((double)LIBXSMM_ALPHA),
        a, *(lda ? lda : LIBXSMM_LD(m, k)), b, *(ldb ? ldb : LIBXSMM_LD(k, n)),
        0 != beta ? *beta : ((double)LIBXSMM_BETA),
        c, *(ldc ? ldc : LIBXSMM_LD(m, n)));
    }
    else {
      LIBXSMM_GEMM_EXTOMP_XGEMM(LIBXSMM_GEMM_EXTOMP_TSK_INIT, LIBXSMM_GEMM_EXTOMP_TSK_LOOP_BEGIN_PARALLEL,
        LIBXSMM_GEMM_EXTOMP_TSK_LOOP_BODY, LIBXSMM_GEMM_EXTOMP_TSK_LOOP_END,
        double, flags, nt, tm, tn, tk, *m, *n, *k,
        0 != alpha ? *alpha : ((double)LIBXSMM_ALPHA),
        a, *(lda ? lda : LIBXSMM_LD(m, k)), b, *(ldb ? ldb : LIBXSMM_LD(k, n)),
        0 != beta ? *beta : ((double)LIBXSMM_BETA),
        c, *(ldc ? ldc : LIBXSMM_LD(m, n)));
    }
  }
  else { /* potentially sequential or externally parallelized */
    if (0 == internal_gemm_tasks) {
      LIBXSMM_GEMM_EXTOMP_XGEMM(LIBXSMM_GEMM_EXTOMP_FOR_INIT, LIBXSMM_GEMM_EXTOMP_FOR_LOOP_BEGIN,
        LIBXSMM_GEMM_EXTOMP_FOR_LOOP_BODY, LIBXSMM_GEMM_EXTOMP_FOR_LOOP_END,
        double, flags, nt, tm, tn, tk, *m, *n, *k,
        0 != alpha ? *alpha : ((double)LIBXSMM_ALPHA),
        a, *(lda ? lda : LIBXSMM_LD(m, k)), b, *(ldb ? ldb : LIBXSMM_LD(k, n)),
        0 != beta ? *beta : ((double)LIBXSMM_BETA),
        c, *(ldc ? ldc : LIBXSMM_LD(m, n)));
    }
    else {
      LIBXSMM_GEMM_EXTOMP_XGEMM(LIBXSMM_GEMM_EXTOMP_TSK_INIT, LIBXSMM_GEMM_EXTOMP_TSK_LOOP_BEGIN,
        LIBXSMM_GEMM_EXTOMP_TSK_LOOP_BODY, LIBXSMM_GEMM_EXTOMP_TSK_LOOP_END,
        double, flags, nt, tm, tn, tk, *m, *n, *k,
        0 != alpha ? *alpha : ((double)LIBXSMM_ALPHA),
        a, *(lda ? lda : LIBXSMM_LD(m, k)), b, *(ldb ? ldb : LIBXSMM_LD(k, n)),
        0 != beta ? *beta : ((double)LIBXSMM_BETA),
        c, *(ldc ? ldc : LIBXSMM_LD(m, n)));
    }
  }
}

