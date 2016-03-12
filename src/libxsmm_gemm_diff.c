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
#include "libxsmm_gemm_diff.h"

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <stdint.h>
#include <stdio.h>
#if !defined(NDEBUG)
# include <assert.h>
#endif
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif
/* must be the last included header */
#include "libxsmm_intrinsics.h"


LIBXSMM_RETARGETABLE LIBXSMM_VISIBILITY_INTERNAL libxsmm_gemm_diff_function internal_gemm_diff_function = libxsmm_gemm_diff;


LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE libxsmm_gemm_diff_function libxsmm_gemm_diff_init(const char* archid, int has_sse)
{
#if !defined(LIBXSMM_GEMM_DIFF_SW)
# if defined(__MIC__)
  internal_gemm_diff_function = libxsmm_gemm_diff_imci;
# elif defined(LIBXSMM_AVX) && (2 <= (LIBXSMM_AVX))
  internal_gemm_diff_function = libxsmm_gemm_diff_avx2;
# elif defined(LIBXSMM_AVX) && (1 <= (LIBXSMM_AVX))
  internal_gemm_diff_function = libxsmm_gemm_diff_avx;
# elif defined(LIBXSMM_SSE) && (3 <= (LIBXSMM_SSE))
  internal_gemm_diff_function = libxsmm_gemm_diff_sse;
# else
  if (0 != archid) {
    if ('h' == archid[0] && 's' == archid[1] && 'w' == archid[2]) {
      internal_gemm_diff_function = libxsmm_gemm_diff_avx2;
    }
    /** 0 != archid is implying at least AVX capabilities */
    else /*if ('s' == archid[0] && 'n' == archid[1] && 'b' == archid[2])*/ {
      internal_gemm_diff_function = libxsmm_gemm_diff_avx;
    }
  }
  else if (0 != has_sse) {
    internal_gemm_diff_function = libxsmm_gemm_diff_sse;
  }
# endif
#endif
  return internal_gemm_diff_function;
}


LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE void libxsmm_gemm_diff_finalize(void)
{
}


LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE
unsigned int libxsmm_gemm_diff(const libxsmm_gemm_descriptor* a, const libxsmm_gemm_descriptor* b)
{
  /* attempt to rely on static code path avoids to rely on capability of inlining pointer-based function call */
#if defined(LIBXSMM_GEMM_DIFF_SW)
  return libxsmm_gemm_diff_sw(a, b);
#elif defined(__MIC__)
  return libxsmm_gemm_diff_imci(a, b);
#elif defined(LIBXSMM_AVX) && (2 <= (LIBXSMM_AVX))
  return libxsmm_gemm_diff_avx2(a, b);
#elif defined(LIBXSMM_AVX) && (1 <= (LIBXSMM_AVX))
  return libxsmm_gemm_diff_avx(a, b);
#elif defined(LIBXSMM_SSE) && (3 <= (LIBXSMM_SSE))
  return libxsmm_gemm_diff_sse(a, b);
#else /* pointer based function call */
  assert(0 != internal_gemm_diff_function);
  return (*internal_gemm_diff_function)(a, b);
#endif
}


LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE
unsigned int libxsmm_gemm_diff_sw(const libxsmm_gemm_descriptor* a, const libxsmm_gemm_descriptor* b)
{
  const unsigned *const ia = (const unsigned int*)a, *const ib = (const unsigned int*)b;
  unsigned int result, i;
  assert(0 == LIBXSMM_MOD2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, sizeof(unsigned int)));
  assert(0 != a && 0 != b);
  result = ia[0] ^ ib[0];
  for (i = 1; i < LIBXSMM_DIV2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, sizeof(unsigned int)); ++i) {
    result |= (ia[i] ^ ib[i]);
  }
  return result;
}


LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE LIBXSMM_INTRINSICS
unsigned int libxsmm_gemm_diff_sse(const libxsmm_gemm_descriptor* a, const libxsmm_gemm_descriptor* b)
{
  return libxsmm_gemm_diff(a, b); /*TODO: SSE based implementation*/
}


LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE LIBXSMM_INTRINSICS
unsigned int libxsmm_gemm_diff_avx(const libxsmm_gemm_descriptor* a, const libxsmm_gemm_descriptor* b)
{
#if defined(LIBXSMM_AVX_MAX) && (1 <= (LIBXSMM_AVX_MAX))
  assert(0 == LIBXSMM_MOD2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, sizeof(unsigned int)));
  assert(8 >= LIBXSMM_DIV2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, 4));
  assert(0 != a && 0 != b);
  {
    int r0, r1;
    __m256i a256, b256;
# if (28 == LIBXSMM_GEMM_DESCRIPTOR_SIZE) /* otherwise generate a compile-time error */
#   if defined(__CYGWIN__) && !defined(NDEBUG) /* Cygwin/GCC: _mm256_set_epi32 may cause an illegal instruction */
    const union { int32_t array[8]; __m256i i; } m256 = { /* use literal value rather than yes/no
      in order to avoid warning about "initializer element is not computable at load time" */
      { 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x0 }
    };
#   else
    const int yes = 0x80000000, no = 0x0;
    struct { __m256i i; } m256;
    m256.i = _mm256_set_epi32(no, yes, yes, yes, yes, yes, yes, yes);
#   endif
# endif
# if defined(LIBXSMM_GEMM_DIFF_MASK_A)
    a256 = _mm256_castps_si256(_mm256_maskload_ps((const float*)a, m256.i));
# else
    /*a256 = _mm256_lddqu_si256((const __m256i*)a);*/
    a256 = _mm256_loadu_si256((const __m256i*)a);
# endif /*defined(LIBXSMM_GEMM_DIFF_MASK_A)*/
    b256 = _mm256_castps_si256(_mm256_maskload_ps((const float*)b, m256.i));
    r0 = _mm256_testnzc_si256(a256, b256);
    r1 = _mm256_testnzc_si256(b256, a256);
    return r0 | r1;
  }
#else
# if !defined(NDEBUG) /* library code is expected to be mute */
  static LIBXSMM_TLS int once = 0;
  if (0 == once) {
    fprintf(stderr, "LIBXSMM: unable to enter AVX instruction code path!\n");
    once = 1;
  }
# endif
# if !defined(__MIC__)
  LIBXSMM_MESSAGE("================================================================================");
  LIBXSMM_MESSAGE("LIBXSMM: Unable to enter the code path which is using AVX instructions!");
  LIBXSMM_MESSAGE("================================================================================");
# endif
  return libxsmm_gemm_diff(a, b);
#endif
}


LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE LIBXSMM_INTRINSICS
unsigned int libxsmm_gemm_diff_avx2(const libxsmm_gemm_descriptor* a, const libxsmm_gemm_descriptor* b)
{
#if defined(LIBXSMM_AVX_MAX) && (2 <= (LIBXSMM_AVX_MAX))
  assert(0 == LIBXSMM_MOD2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, sizeof(unsigned int)));
  assert(8 >= LIBXSMM_DIV2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, 4));
  assert(0 != a && 0 != b);
  {
# if (28 == LIBXSMM_GEMM_DESCRIPTOR_SIZE) /* otherwise generate a compile-time error */
    const int yes = 0x80000000, no = 0x0;
    const __m256i m256 = _mm256_set_epi32(no, yes, yes, yes, yes, yes, yes, yes);
# endif
# if defined(LIBXSMM_GEMM_DIFF_MASK_A)
    const __m256i a256 = _mm256_maskload_epi32((const void*)a, m256);
# else
    /*const __m256i a256 = _mm256_lddqu_si256((const __m256i*)a);*/
    const __m256i a256 = _mm256_loadu_si256((const __m256i*)a);
# endif /*defined(LIBXSMM_GEMM_DIFF_MASK_A)*/
    const __m256i b256 = _mm256_maskload_epi32((const void*)b, m256);
    const int r0 = _mm256_testnzc_si256(a256, b256);
    const int r1 = _mm256_testnzc_si256(b256, a256);
    return r0 | r1;
  }
#else
# if !defined(NDEBUG) /* library code is expected to be mute */
  static LIBXSMM_TLS int once = 0;
  if (0 == once) {
    fprintf(stderr, "LIBXSMM: unable to enter AVX2 instruction code path!\n");
    once = 1;
  }
# endif
# if !defined(__MIC__)
  LIBXSMM_MESSAGE("================================================================================");
  LIBXSMM_MESSAGE("LIBXSMM: Unable to enter the code path which is using AVX2 instructions!");
  LIBXSMM_MESSAGE("================================================================================");
# endif
  return libxsmm_gemm_diff(a, b);
#endif
}


#if defined(__MIC__)
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE
unsigned int libxsmm_gemm_diff_imci(const libxsmm_gemm_descriptor* a, const libxsmm_gemm_descriptor* b)
{
  assert(0 == LIBXSMM_MOD2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, sizeof(unsigned int)));
  assert(16 >= LIBXSMM_DIV2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, 4));
  assert(0 != a && 0 != b);
  {
    const __mmask16 mask = (0xFFFF >> (16 - LIBXSMM_DIV2(LIBXSMM_GEMM_DESCRIPTOR_SIZE, 4)));
    const __m512i a512 = _mm512_mask_loadunpackhi_epi32(
      _mm512_mask_loadunpacklo_epi32(_mm512_set1_epi32(0), mask, a),
      mask, ((const char*)a) + 32);
    const __m512i b512 = _mm512_mask_loadunpackhi_epi32(
      _mm512_mask_loadunpacklo_epi32(_mm512_set1_epi32(0), mask, b),
      mask, ((const char*)b) + 32);
    return _mm512_reduce_or_epi32(_mm512_xor_si512(a512, b512));
  }
}
#endif /*defined(__MIC__)*/

