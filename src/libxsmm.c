/******************************************************************************
** Copyright (c) 2014-2016, Intel Corporation                                **
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
/* Hans Pabst (Intel Corp.), Alexander Heinecke (Intel Corp.)
******************************************************************************/
#include "libxsmm_intrinsics_x86.h"
#include "libxsmm_cpuid_x86.h"
#include "libxsmm_gemm_diff.h"
#include "libxsmm_alloc.h"
#include "libxsmm_gemm.h"
#include "libxsmm_hash.h"
#include "libxsmm_sync.h"

#if defined(__TRACE)
# include "libxsmm_trace.h"
#endif

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
/* mute warning about target attribute; KNC/native plus JIT is disabled below! */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#if !defined(NDEBUG)
# include <assert.h>
# include <errno.h>
#endif
#if defined(_WIN32)
# include <Windows.h>
#else
# include <sys/mman.h>
# include <pthread.h>
# include <unistd.h>
# include <fcntl.h>
#endif
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

/**
 * LIBXSMM is agnostic with respect to the threading runtime!
 * LIBXSMM_OPENMP suppresses using OS primitives (PThreads)
 */
#if defined(_OPENMP) && !defined(LIBXSMM_OPENMP)
/*# define LIBXSMM_OPENMP*/
#endif

/* alternative hash algorithm (instead of CRC32) */
#if !defined(LIBXSMM_HASH_BASIC) && !defined(LIBXSMM_REGSIZE)
# if !defined(LIBXSMM_MAX_STATIC_TARGET_ARCH) || (LIBXSMM_X86_SSE4_2 > LIBXSMM_MAX_STATIC_TARGET_ARCH)
/*#   define LIBXSMM_HASH_BASIC*/
# endif
#endif

/* allow external definition to enable testing corner cases (exhausted registry space) */
#if !defined(LIBXSMM_REGSIZE)
# if defined(LIBXSMM_HASH_BASIC) /* consider larger registry to better deal with low-quality hash */
#   define LIBXSMM_REGSIZE /*1048576*/524288 /* no Mersenne Prime number required, but POT number */
# else
#   define LIBXSMM_REGSIZE 524288 /* 524287: Mersenne Prime number (2^19-1) */
# endif
# define LIBXSMM_HASH_MOD(N, NPOT) LIBXSMM_MOD2(N, NPOT)
#else
# define LIBXSMM_HASH_MOD(N, NGEN) ((N) % (NGEN))
#endif

#if !defined(LIBXSMM_CACHESIZE)
# define LIBXSMM_CACHESIZE 4
#endif

#if defined(LIBXSMM_HASH_BASIC)
# define LIBXSMM_HASH_FUNCTION_CALL(HASH, INDX, DESCRIPTOR) \
    HASH = libxsmm_hash_npot(&(DESCRIPTOR), LIBXSMM_GEMM_DESCRIPTOR_SIZE, LIBXSMM_REGSIZE); \
    assert((LIBXSMM_REGSIZE) > (HASH)); \
    INDX = (HASH)
#else
# define LIBXSMM_HASH_FUNCTION_CALL(HASH, INDX, DESCRIPTOR) \
    HASH = libxsmm_crc32(&(DESCRIPTOR), LIBXSMM_GEMM_DESCRIPTOR_SIZE, 25071975/*seed*/); \
    INDX = LIBXSMM_HASH_MOD(HASH, LIBXSMM_REGSIZE)
#endif

/* flag fused into the memory address of a code version in case of collision */
#define LIBXSMM_HASH_COLLISION (1ULL << (8 * sizeof(void*) - 1))
/* flag fused into the memory address of a code version in case of non-JIT */
#define LIBXSMM_CODE_STATIC (1ULL << (8 * sizeof(void*) - 2))

#if 16 >= (LIBXSMM_GEMM_DESCRIPTOR_SIZE)
# define LIBXSMM_GEMM_DESCRIPTOR_SIMD_SIZE 16
#elif 32 >= (LIBXSMM_GEMM_DESCRIPTOR_SIZE)
# define LIBXSMM_GEMM_DESCRIPTOR_SIMD_SIZE 32
#else
# define LIBXSMM_GEMM_DESCRIPTOR_SIMD_SIZE LIBXSMM_GEMM_DESCRIPTOR_SIZE
#endif

typedef union LIBXSMM_RETARGETABLE internal_regkey_type {
  char simd[LIBXSMM_GEMM_DESCRIPTOR_SIMD_SIZE];
  libxsmm_gemm_descriptor descriptor;
} internal_regkey_type;

typedef union LIBXSMM_RETARGETABLE internal_code_type {
  libxsmm_xmmfunction xmm;
  /*const*/void* pmm;
  uintptr_t imm;
} internal_code_type;

typedef struct LIBXSMM_RETARGETABLE internal_statistic_type {
  unsigned int ntry, ncol, njit, nsta;
} internal_statistic_type;

typedef struct LIBXSMM_RETARGETABLE internal_desc_extra_type {
  const unsigned int* row_ptr;
  const unsigned int* column_idx;
  const void* values;
} internal_desc_extra_type;

/** Helper macro determining the default prefetch strategy which is used for statically generated kernels. */
#if defined(_WIN32) || defined(__CYGWIN__) /*TODO: account for calling convention; avoid passing six arguments*/
# define INTERNAL_PREFETCH LIBXSMM_PREFETCH_NONE
#elif defined(__MIC__) && (0 > LIBXSMM_PREFETCH) /* auto-prefetch (frontend) */
# define INTERNAL_PREFETCH LIBXSMM_PREFETCH_AL2BL2_VIA_C
#elif (0 > LIBXSMM_PREFETCH) /* auto-prefetch (frontend) */
# define INTERNAL_PREFETCH LIBXSMM_PREFETCH_SIGONLY
#endif
#if !defined(INTERNAL_PREFETCH)
# define INTERNAL_PREFETCH LIBXSMM_PREFETCH
#endif

#if !defined(LIBXSMM_TRYLOCK)
/*# define LIBXSMM_TRYLOCK*/
#endif

#if defined(__GNUC__)
/* libxsmm_init already executed via GCC constructor attribute */
# define INTERNAL_FIND_CODE_INIT(VARIABLE) assert(0 != (VARIABLE))
#else /* lazy initialization */
/* use return value of internal_init to refresh local representation */
# define INTERNAL_FIND_CODE_INIT(VARIABLE) if (0 == (VARIABLE)) VARIABLE = internal_init()
#endif

#if defined(LIBXSMM_OPENMP)
# define INTERNAL_FIND_CODE_LOCK(LOCKINDEX, INDEX) LIBXSMM_PRAGMA(omp critical(internal_reglock)) { \
# define INTERNAL_FIND_CODE_UNLOCK(LOCKINDEX) }
#elif !defined(LIBXSMM_NOSYNC)
# if defined(LIBXSMM_TRYLOCK)
#   define INTERNAL_FIND_CODE_LOCK(LOCKINDEX, INDEX) { \
      const unsigned int LOCKINDEX = LIBXSMM_MOD2(INDEX, INTERNAL_REGLOCK_COUNT); \
      if (LIBXSMM_LOCK_ACQUIRED != LIBXSMM_LOCK_TRYLOCK(internal_reglock + (LOCKINDEX))) { \
        assert(0 == diff); continue; \
      }
# else
#   define INTERNAL_FIND_CODE_LOCK(LOCKINDEX, INDEX) { \
      const unsigned int LOCKINDEX = LIBXSMM_MOD2(INDEX, INTERNAL_REGLOCK_COUNT); \
      LIBXSMM_LOCK_ACQUIRE(internal_reglock + (LOCKINDEX))
# endif
# define INTERNAL_FIND_CODE_UNLOCK(LOCKINDEX) LIBXSMM_LOCK_RELEASE(internal_reglock + (LOCKINDEX)); }
#else
# define INTERNAL_FIND_CODE_LOCK(LOCKINDEX, INDEX)
# define INTERNAL_FIND_CODE_UNLOCK(LOCKINDEX)
#endif

#define INTERNAL_FIND_CODE_DECLARE(CODE) internal_code_type* CODE = \
  LIBXSMM_ATOMIC_LOAD(&internal_registry, LIBXSMM_ATOMIC_RELAXED); unsigned int i

#if defined(LIBXSMM_CACHESIZE) && (0 < (LIBXSMM_CACHESIZE))
# define INTERNAL_FIND_CODE_CACHE_DECL(CACHE_ID, CACHE_KEYS, CACHE, CACHE_HIT) \
  static LIBXSMM_TLS union { libxsmm_gemm_descriptor desc; char padding[LIBXSMM_GEMM_DESCRIPTOR_SIMD_SIZE]; } CACHE_KEYS[LIBXSMM_CACHESIZE]; \
  static LIBXSMM_TLS libxsmm_xmmfunction CACHE[LIBXSMM_CACHESIZE]; \
  static LIBXSMM_TLS unsigned int CACHE_ID = (unsigned int)(-1); \
  static LIBXSMM_TLS unsigned int CACHE_HIT = LIBXSMM_CACHESIZE;
# define INTERNAL_FIND_CODE_CACHE_BEGIN(CACHE_ID, CACHE_KEYS, CACHE, CACHE_HIT, RESULT, DESCRIPTOR) \
  assert(LIBXSMM_GEMM_DESCRIPTOR_SIMD_SIZE >= LIBXSMM_GEMM_DESCRIPTOR_SIZE); \
  /* search small cache starting with the last hit on record */ \
  i = libxsmm_gemm_diffn(DESCRIPTOR, &(CACHE_KEYS)->desc, CACHE_HIT, LIBXSMM_CACHESIZE, LIBXSMM_GEMM_DESCRIPTOR_SIMD_SIZE); \
  if ((LIBXSMM_CACHESIZE) > i && (CACHE_ID) == internal_teardown) { /* cache hit, and valid */ \
    (RESULT).xmm = (CACHE)[i]; \
    CACHE_HIT = i; \
  } \
  else
# if defined(LIBXSMM_GEMM_DIFF_SW) && (2 == (LIBXSMM_GEMM_DIFF_SW)) /* most general implementation */
#   define INTERNAL_FIND_CODE_CACHE_FINALIZE(CACHE_ID, CACHE_KEYS, CACHE, CACHE_HIT, RESULT, DESCRIPTOR) \
    if ((CACHE_ID) != internal_teardown) { \
      memset(CACHE_KEYS, -1, sizeof(CACHE_KEYS)); \
      CACHE_ID = internal_teardown; \
    } \
    i = ((CACHE_HIT) + ((LIBXSMM_CACHESIZE) - 1)) % (LIBXSMM_CACHESIZE); \
    ((CACHE_KEYS)[i]).desc = *(DESCRIPTOR); \
    (CACHE)[i] = (RESULT).xmm; \
    CACHE_HIT = i
# else
#   define INTERNAL_FIND_CODE_CACHE_FINALIZE(CACHE_ID, CACHE_KEYS, CACHE, CACHE_HIT, RESULT, DESCRIPTOR) \
    assert(/*is pot*/(LIBXSMM_CACHESIZE) == (1 << LIBXSMM_LOG2(LIBXSMM_CACHESIZE))); \
    if ((CACHE_ID) != internal_teardown) { \
      memset(CACHE_KEYS, -1, sizeof(CACHE_KEYS)); \
      CACHE_ID = internal_teardown; \
    } \
    i = LIBXSMM_MOD2((CACHE_HIT) + ((LIBXSMM_CACHESIZE) - 1), LIBXSMM_CACHESIZE); \
    (CACHE_KEYS)[i].desc = *(DESCRIPTOR); \
    (CACHE)[i] = (RESULT).xmm; \
    CACHE_HIT = i
# endif
#else
# define INTERNAL_FIND_CODE_CACHE_DECL(CACHE_ID, CACHE_KEYS, CACHE, CACHE_HIT)
# define INTERNAL_FIND_CODE_CACHE_BEGIN(CACHE_ID, CACHE_KEYS, CACHE, CACHE_HIT, RESULT, DESCRIPTOR)
# define INTERNAL_FIND_CODE_CACHE_FINALIZE(CACHE_ID, CACHE_KEYS, CACHE, CACHE_HIT, RESULT, DESCRIPTOR)
#endif

#if (0 != LIBXSMM_JIT)
# define INTERNAL_FIND_CODE_JIT(DESCRIPTOR, CODE, RESULT) \
  /* check if code generation or fix-up is needed, also check whether JIT is supported (CPUID) */ \
  if (0 == (RESULT).pmm/*code version does not exist*/ && LIBXSMM_X86_AVX <= internal_target_archid) { \
    /* instead of blocking others, a try-lock allows to let other threads fallback to BLAS during lock-duration */ \
    INTERNAL_FIND_CODE_LOCK(lock, i); /* lock the registry entry */ \
    if (0 == diff) { \
      RESULT = *(CODE); /* deliver code */ \
      /* clear collision flag; can be never static code */ \
      assert(0 == (LIBXSMM_CODE_STATIC & (RESULT).imm)); \
      (RESULT).imm &= ~LIBXSMM_HASH_COLLISION; \
    } \
    if (0 == (RESULT).pmm) { /* double-check (re-read registry entry) after acquiring the lock */ \
      if (0 == diff) { \
        /* found a conflict-free registry-slot, and attempt to build the kernel */ \
        internal_build(DESCRIPTOR, "smm", 0/*extra desc*/, &(RESULT)); \
        internal_update_statistic(DESCRIPTOR, 1, 0); \
        if (0 != (RESULT).pmm) { /* synchronize registry entry */ \
          internal_registry_keys[i].descriptor = *(DESCRIPTOR); \
          *(CODE) = RESULT; \
          LIBXSMM_ATOMIC_STORE(&(CODE)->pmm, (RESULT).pmm, LIBXSMM_ATOMIC_SEQ_CST); \
        } \
      } \
      else { /* 0 != diff */ \
        if (0 == diff0 && /*bypass*/0 == (LIBXSMM_HASH_COLLISION & (CODE)->imm)) { \
          /* flag existing entry as collision */ \
          internal_code_type collision; \
          /* find new slot to store the code version */ \
          const unsigned int index = LIBXSMM_HASH_MOD(LIBXSMM_HASH_VALUE(hash), LIBXSMM_REGSIZE); \
          collision.imm = (CODE)->imm | LIBXSMM_HASH_COLLISION; \
          i = (index != i ? index : LIBXSMM_HASH_MOD(index + 1, LIBXSMM_REGSIZE)); \
          i0 = i; /* keep starting point of free-slot-search in mind */ \
          internal_update_statistic(DESCRIPTOR, 0, 1); \
          LIBXSMM_ATOMIC_STORE(&(CODE)->pmm, collision.pmm, LIBXSMM_ATOMIC_SEQ_CST); /* fix-up existing entry */ \
          diff0 = diff; /* no more fix-up */ \
        } \
        else { \
          const unsigned int next = LIBXSMM_HASH_MOD(i + 1, LIBXSMM_REGSIZE); \
          if (next != i0) { /* linear search for free slot not completed */ \
            i = next; \
          } \
          else { /* out of registry capacity (no free slot found) */ \
            diff = 0; \
          } \
        } \
        (CODE) = internal_registry + i; \
      } \
    } \
    INTERNAL_FIND_CODE_UNLOCK(lock); \
  } \
  else
#else
# define INTERNAL_FIND_CODE_JIT(DESCRIPTOR, CODE, RESULT)
#endif

#define INTERNAL_FIND_CODE(DESCRIPTOR, CODE) \
  internal_code_type flux_entry; \
{ \
  INTERNAL_FIND_CODE_CACHE_DECL(cache_id, cache_keys, cache, cache_hit) \
  unsigned int hash, diff = 0, diff0 = 0, i0; \
  INTERNAL_FIND_CODE_INIT(CODE); \
  INTERNAL_FIND_CODE_CACHE_BEGIN(cache_id, cache_keys, cache, cache_hit, flux_entry, DESCRIPTOR) { \
    /* check if the requested xGEMM is already JITted */ \
    LIBXSMM_PRAGMA_FORCEINLINE /* must precede a statement */ \
    LIBXSMM_HASH_FUNCTION_CALL(hash, i = i0, *(DESCRIPTOR)); \
    (CODE) += i; /* actual entry */ \
    do { \
      flux_entry.pmm = LIBXSMM_ATOMIC_LOAD(&(CODE)->pmm, LIBXSMM_ATOMIC_SEQ_CST); /* read registered code */ \
      if (0 != flux_entry.pmm) { /* code version exists */ \
        if (0 == diff0) { \
          if (0 == (LIBXSMM_HASH_COLLISION & flux_entry.imm)) { /* check for no collision */ \
            /* calculate bitwise difference (deep check) */ \
            LIBXSMM_PRAGMA_FORCEINLINE /* must precede a statement */ \
            diff = libxsmm_gemm_diff(DESCRIPTOR, &internal_registry_keys[i].descriptor); \
            if (0 != diff) { /* new collision discovered (but no code version yet) */ \
              /* allow to fix-up current entry inside of the guarded/locked region */ \
              flux_entry.pmm = 0; \
            } \
            else { \
              flux_entry.imm &= ~LIBXSMM_CODE_STATIC; \
            } \
          } \
          /* collision discovered but code version exists; perform deep check */ \
          else if (0 != libxsmm_gemm_diff(DESCRIPTOR, &internal_registry_keys[i].descriptor)) { \
            /* continue linearly searching code starting at re-hashed index position */ \
            const unsigned int index = LIBXSMM_HASH_MOD(LIBXSMM_HASH_VALUE(hash), LIBXSMM_REGSIZE); \
            unsigned int next; \
            for (i0 = (index != i ? index : LIBXSMM_HASH_MOD(index + 1, LIBXSMM_REGSIZE)), \
              i = i0, next = LIBXSMM_HASH_MOD(i0 + 1, LIBXSMM_REGSIZE); \
              /* skip entries which correspond to no code, or continue on difference */ \
              (0 == (CODE = (internal_registry + i))->pmm || \
                0 != (diff = libxsmm_gemm_diff(DESCRIPTOR, &internal_registry_keys[i].descriptor))) \
                /* entire registry was searched and no code version was found */ \
                && next != i0; \
              i = next, next = LIBXSMM_HASH_MOD(i + 1, LIBXSMM_REGSIZE)); \
            if (0 == diff) { /* found exact code version; continue with atomic load */ \
              flux_entry.pmm = (CODE)->pmm; \
              /* clear the collision and the non-JIT flag */ \
              flux_entry.imm &= ~(LIBXSMM_HASH_COLLISION | LIBXSMM_CODE_STATIC); \
            } \
            else { /* no code found */ \
              flux_entry.pmm = 0; \
            } \
            break; \
          } \
          else { /* clear the collision and the non-JIT flag */ \
            flux_entry.imm &= ~(LIBXSMM_HASH_COLLISION | LIBXSMM_CODE_STATIC); \
          } \
        } \
        else { /* new collision discovered (but no code version yet) */ \
          flux_entry.pmm = 0; \
        } \
      } \
      INTERNAL_FIND_CODE_JIT(DESCRIPTOR, CODE, flux_entry) \
      { \
        diff = 0; \
      } \
    } \
    while (0 != diff); \
    assert(0 == diff || 0 == flux_entry.pmm); \
    INTERNAL_FIND_CODE_CACHE_FINALIZE(cache_id, cache_keys, cache, cache_hit, flux_entry, DESCRIPTOR); \
  } \
} \
return flux_entry.xmm

#define INTERNAL_DISPATCH_BYPASS_CHECK(FLAGS, ALPHA, BETA) ( \
  0 == ((FLAGS) & (LIBXSMM_GEMM_FLAG_TRANS_A | LIBXSMM_GEMM_FLAG_TRANS_B)) && \
  1 == (ALPHA) && (1 == (BETA) || 0 == (BETA)))

#define INTERNAL_DISPATCH_MAIN(DESCRIPTOR_DECL, DESC, FLAGS, M, N, K, PLDA, PLDB, PLDC, PALPHA, PBETA, PREFETCH, SELECTOR/*smm or dmm*/) { \
  INTERNAL_FIND_CODE_DECLARE(code); \
  const signed char scalpha = (signed char)(0 == (PALPHA) ? LIBXSMM_ALPHA : *(PALPHA)), scbeta = (signed char)(0 == (PBETA) ? LIBXSMM_BETA : *(PBETA)); \
  if (INTERNAL_DISPATCH_BYPASS_CHECK(FLAGS, scalpha, scbeta)) { \
    const int internal_dispatch_main_prefetch = (0 == (PREFETCH) ? INTERNAL_PREFETCH : *(PREFETCH)); \
    DESCRIPTOR_DECL; LIBXSMM_GEMM_DESCRIPTOR(*(DESC), 0 != (VECTOR_WIDTH) ? (VECTOR_WIDTH): LIBXSMM_ALIGNMENT, \
      FLAGS, LIBXSMM_LD(M, N), LIBXSMM_LD(N, M), K, \
      0 == LIBXSMM_LD(PLDA, PLDB) ? LIBXSMM_LD(M, N) : *LIBXSMM_LD(PLDA, PLDB), \
      0 == LIBXSMM_LD(PLDB, PLDA) ? (K) : *LIBXSMM_LD(PLDB, PLDA), \
      0 == (PLDC) ? LIBXSMM_LD(M, N) : *(PLDC), scalpha, scbeta, \
      0 > internal_dispatch_main_prefetch ? internal_prefetch : internal_dispatch_main_prefetch); \
    { \
      INTERNAL_FIND_CODE(DESC, code).SELECTOR; \
    } \
  } \
  else { /* TODO: not supported (bypass) */ \
    return 0; \
  } \
}

#if defined(LIBXSMM_GEMM_DIFF_MASK_A) /* no padding i.e., LIBXSMM_GEMM_DESCRIPTOR_SIZE */
# define INTERNAL_DISPATCH(FLAGS, M, N, K, PLDA, PLDB, PLDC, PALPHA, PBETA, PREFETCH, SELECTOR/*smm or dmm*/) \
    INTERNAL_DISPATCH_MAIN(libxsmm_gemm_descriptor descriptor, &descriptor, \
    FLAGS, M, N, K, PLDA, PLDB, PLDC, PALPHA, PBETA, PREFETCH, SELECTOR/*smm or dmm*/)
#else /* padding: LIBXSMM_GEMM_DESCRIPTOR_SIZE -> LIBXSMM_ALIGNMENT */
# define INTERNAL_DISPATCH(FLAGS, M, N, K, PLDA, PLDB, PLDC, PALPHA, PBETA, PREFETCH, SELECTOR/*smm or dmm*/) { \
    INTERNAL_DISPATCH_MAIN(union { libxsmm_gemm_descriptor desc; char simd[LIBXSMM_ALIGNMENT]; } simd_descriptor; \
      for (i = LIBXSMM_GEMM_DESCRIPTOR_SIZE; i < sizeof(simd_descriptor.simd); ++i) simd_descriptor.simd[i] = 0, &simd_descriptor.desc, \
    FLAGS, M, N, K, PLDA, PLDB, PLDC, PALPHA, PBETA, PREFETCH, SELECTOR/*smm or dmm*/)
#endif

#define INTERNAL_SMMDISPATCH(PFLAGS, M, N, K, PLDA, PLDB, PLDC, PALPHA, PBETA, PREFETCH) \
  INTERNAL_DISPATCH((0 == (PFLAGS) ? LIBXSMM_FLAGS : *(PFLAGS)) | LIBXSMM_GEMM_FLAG_F32PREC, \
  M, N, K, PLDA, PLDB, PLDC, PALPHA, PBETA, PREFETCH, smm)
#define INTERNAL_DMMDISPATCH(PFLAGS, M, N, K, PLDA, PLDB, PLDC, PALPHA, PBETA, PREFETCH) \
  INTERNAL_DISPATCH((0 == (PFLAGS) ? LIBXSMM_FLAGS : *(PFLAGS)), \
  M, N, K, PLDA, PLDB, PLDC, PALPHA, PBETA, PREFETCH, dmm)

#if !defined(LIBXSMM_OPENMP) && !defined(LIBXSMM_NOSYNC)
# define INTERNAL_REGLOCK_COUNT 16
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE LIBXSMM_LOCK_TYPE internal_reglock[INTERNAL_REGLOCK_COUNT];
#endif

LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE internal_regkey_type* internal_registry_keys /*= 0*/;
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE internal_code_type* internal_registry /*= 0*/;
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE internal_statistic_type internal_statistic[2/*DP/SP*/][3/*sml/med/big*/];
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE unsigned int internal_statistic_sml /*= 13*/;
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE unsigned int internal_statistic_med /*= 23*/;
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE unsigned int internal_statistic_mnk /*= LIBXSMM_MAX_M*/;
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE unsigned int internal_teardown /*= 0*/;
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE int internal_target_archid /*= LIBXSMM_TARGET_ARCH_GENERIC*/;
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE int internal_verbose_mode /*= 0*/;
LIBXSMM_EXTERN_C LIBXSMM_RETARGETABLE int internal_prefetch /*= LIBXSMM_MAX(INTERNAL_PREFETCH, 0)*/;


LIBXSMM_INLINE LIBXSMM_RETARGETABLE void internal_update_statistic(const libxsmm_gemm_descriptor* desc,
  unsigned ntry, unsigned ncol)
{
  assert(0 != desc);
  {
    const unsigned long long size = LIBXSMM_MNK_SIZE(desc->m, desc->n, desc->k);
    const int precision = (0 == (LIBXSMM_GEMM_FLAG_F32PREC & desc->flags) ? 0 : 1);
    const unsigned int statistic_sml = internal_statistic_sml;
    int bucket = 2/*big*/;

    if (LIBXSMM_MNK_SIZE(statistic_sml, statistic_sml, statistic_sml) >= size) {
      bucket = 0;
    }
    else {
      const unsigned int statistic_med = internal_statistic_med;
      if (LIBXSMM_MNK_SIZE(statistic_med, statistic_med, statistic_med) >= size) {
        bucket = 1;
      }
    }

    LIBXSMM_ATOMIC_ADD_FETCH(&internal_statistic[precision][bucket].ntry, ntry, LIBXSMM_ATOMIC_RELAXED);
    LIBXSMM_ATOMIC_ADD_FETCH(&internal_statistic[precision][bucket].ncol, ncol, LIBXSMM_ATOMIC_RELAXED);
  }
}


LIBXSMM_INLINE LIBXSMM_RETARGETABLE const char* internal_get_target_arch(int id);
LIBXSMM_INLINE LIBXSMM_RETARGETABLE const char* internal_get_target_arch(int id)
{
  const char* target_arch = 0;
  switch (id) {
    case LIBXSMM_X86_AVX512_CORE: {
      target_arch = "skx";
    } break;
    case LIBXSMM_X86_AVX512_MIC: {
      target_arch = "knl";
    } break;
    case LIBXSMM_X86_AVX2: {
      target_arch = "hsw";
    } break;
    case LIBXSMM_X86_AVX: {
      target_arch = "snb";
    } break;
    case LIBXSMM_X86_SSE4_2: {
      target_arch = "wsm";
    } break;
    case LIBXSMM_X86_SSE4_1: {
      target_arch = "sse4";
    } break;
    case LIBXSMM_X86_SSE3: {
      target_arch = "sse3";
    } break;
    case LIBXSMM_TARGET_ARCH_GENERIC: {
      target_arch = "generic";
    } break;
    default: if (LIBXSMM_X86_GENERIC <= id) {
      target_arch = "x86";
    }
    else {
      target_arch = "unknown";
    }
  }

  assert(0 != target_arch);
  return target_arch;
}


LIBXSMM_INLINE LIBXSMM_RETARGETABLE unsigned int internal_print_statistic(FILE* ostream, const char* target_arch, int precision, unsigned int linebreaks, unsigned int indent)
{
  const internal_statistic_type statistic_sml = internal_statistic[precision][0/*SML*/];
  const internal_statistic_type statistic_med = internal_statistic[precision][1/*MED*/];
  const internal_statistic_type statistic_big = internal_statistic[precision][2/*BIG*/];
  int printed = 0;
  assert(0 != ostream && 0 != target_arch && (0 <= precision && precision < 2));

  if (/* omit to print anything if it is superfluous */
    0 != statistic_sml.ntry || 0 != statistic_sml.njit || 0 != statistic_sml.nsta || 0 != statistic_sml.ncol ||
    0 != statistic_med.ntry || 0 != statistic_med.njit || 0 != statistic_med.nsta || 0 != statistic_med.ncol ||
    0 != statistic_big.ntry || 0 != statistic_big.njit || 0 != statistic_big.nsta || 0 != statistic_big.ncol)
  {
    const unsigned int sml = internal_statistic_sml, med = internal_statistic_med, mnk = internal_statistic_mnk;
    char title[256], csml[256], cmed[256], cbig[256];
    LIBXSMM_SNPRINTF(csml, sizeof(csml), "%u..%u",       0u, sml);
    LIBXSMM_SNPRINTF(cmed, sizeof(cmed), "%u..%u", sml + 1u, med);
    LIBXSMM_SNPRINTF(cbig, sizeof(cbig), "%u..%u", med + 1u, mnk);
    {
      unsigned int n = 0;
      for (n = 0; 0 != target_arch[n] && n < sizeof(title); ++n) { /* toupper */
        const char c = target_arch[n];
        title[n] = (char)(('a' <= c && c <= 'z') ? (c - 32) : c);
      }
      LIBXSMM_SNPRINTF(title + n, sizeof(title) - n, "/%s", 0 == precision ? "DP" : "SP");
      for (n = 0; n < linebreaks; ++n) fprintf(ostream, "\n");
    }
    fprintf(ostream, "%*s%-10s %6s %6s %6s %6s\n", (int)indent, "", title, "TRY" ,"JIT", "STA", "COL");
    fprintf(ostream,  "%*s%10s %6u %6u %6u %6u\n", (int)indent, "", csml,
      statistic_sml.ntry, statistic_sml.njit, statistic_sml.nsta, statistic_sml.ncol);
    fprintf(ostream,  "%*s%10s %6u %6u %6u %6u\n", (int)indent, "", cmed,
      statistic_med.ntry, statistic_med.njit, statistic_med.nsta, statistic_med.ncol);
    fprintf(ostream,  "%*s%10s %6u %6u %6u %6u\n", (int)indent, "", cbig,
      statistic_big.ntry, statistic_big.njit, statistic_big.nsta, statistic_big.ncol);
    printed = 1;
  }

  return printed;
}


LIBXSMM_INLINE LIBXSMM_RETARGETABLE void internal_register_static_code(const libxsmm_gemm_descriptor* desc,
  unsigned int index, unsigned int hash, libxsmm_xmmfunction src, internal_code_type* registry)
{
  internal_regkey_type* dst_key = internal_registry_keys + index;
  internal_code_type* dst_entry = registry + index;
#if !defined(NDEBUG)
  internal_code_type code; code.xmm = src;
  assert(0 != desc && 0 != code.pmm && 0 != dst_key && 0 != registry);
  assert(0 == ((LIBXSMM_HASH_COLLISION | LIBXSMM_CODE_STATIC) & code.imm));
#endif

  if (0 != dst_entry->pmm) { /* collision? */
    /* start at a re-hashed index position */
    const unsigned int start = LIBXSMM_HASH_MOD(LIBXSMM_HASH_VALUE(hash), LIBXSMM_REGSIZE);
    unsigned int i0, i, next;

    /* mark current entry as a collision (this might be already the case) */
    dst_entry->imm |= LIBXSMM_HASH_COLLISION;

    /* start linearly searching for an available slot */
    for (i = (start != index) ? start : LIBXSMM_HASH_MOD(start + 1, LIBXSMM_REGSIZE), i0 = i, next = LIBXSMM_HASH_MOD(i + 1, LIBXSMM_REGSIZE);
      0 != (dst_entry = registry + i)->pmm && next != i0; i = next, next = LIBXSMM_HASH_MOD(i + 1, LIBXSMM_REGSIZE));

    /* corresponding key position */
    dst_key = internal_registry_keys + i;

    internal_update_statistic(desc, 0, 1);
  }

  if (0 == dst_entry->pmm) { /* registry not (yet) exhausted */
    dst_key->descriptor = *desc;
    dst_entry->xmm = src;
    /* mark current entry as a static (non-JIT) */
    dst_entry->imm |= LIBXSMM_CODE_STATIC;
  }

  internal_update_statistic(desc, 1, 0);
}


LIBXSMM_INLINE LIBXSMM_RETARGETABLE int internal_get_prefetch(const libxsmm_gemm_descriptor* desc)
{
  assert(0 != desc);
  switch (desc->prefetch) {
    case LIBXSMM_PREFETCH_SIGONLY:            return 2;
    case LIBXSMM_PREFETCH_BL2_VIA_C:          return 3;
    case LIBXSMM_PREFETCH_AL2:                return 4;
    case LIBXSMM_PREFETCH_AL2_AHEAD:          return 5;
    case LIBXSMM_PREFETCH_AL2BL2_VIA_C:       return 6;
    case LIBXSMM_PREFETCH_AL2BL2_VIA_C_AHEAD: return 7;
    case LIBXSMM_PREFETCH_AL2_JPST:           return 8;
    case LIBXSMM_PREFETCH_AL2BL2_VIA_C_JPST:  return 9;
    default: {
      assert(LIBXSMM_PREFETCH_NONE == desc->prefetch);
      return 0;
    }
  }
}


LIBXSMM_INLINE LIBXSMM_RETARGETABLE void internal_get_code_name(const char* target_arch, const char* jit_kind,
  const libxsmm_gemm_descriptor* desc, unsigned int buffer_size, char* name)
{
  assert((0 != desc && 0 != name) || 0 == buffer_size);
  LIBXSMM_SNPRINTF(name, buffer_size, "libxsmm_%s_%c%c%c_%ux%ux%u_%u_%u_%u_a%i_b%i_p%i.%s",
    target_arch /* code path name */,
    0 == (LIBXSMM_GEMM_FLAG_F32PREC & desc->flags) ? 'd' : 's',
    0 == (LIBXSMM_GEMM_FLAG_TRANS_A & desc->flags) ? 'n' : 't',
    0 == (LIBXSMM_GEMM_FLAG_TRANS_B & desc->flags) ? 'n' : 't',
    (unsigned int)desc->m, (unsigned int)desc->n, (unsigned int)desc->k,
    (unsigned int)desc->lda, (unsigned int)desc->ldb, (unsigned int)desc->ldc,
    desc->alpha, desc->beta, internal_get_prefetch(desc),
    0 != jit_kind ? jit_kind : "jit");
}


LIBXSMM_INLINE LIBXSMM_RETARGETABLE internal_code_type* internal_init(void)
{
  /*const*/internal_code_type* result;
  int i;
#if !defined(LIBXSMM_OPENMP)
# if !defined(LIBXSMM_NOSYNC)
  static int internal_reglock_check = 1; /* setup the locks in a thread-safe fashion */
  assert(sizeof(internal_reglock) == (INTERNAL_REGLOCK_COUNT * sizeof(*internal_reglock)));
  if (1 == LIBXSMM_ATOMIC_LOAD(&internal_reglock_check, LIBXSMM_ATOMIC_SEQ_CST)) {
    LIBXSMM_ATOMIC_ADD_FETCH(&internal_reglock_check, 1, LIBXSMM_ATOMIC_SEQ_CST);
    if (2 == internal_reglock_check) {
      for (i = 0; i < INTERNAL_REGLOCK_COUNT; ++i) LIBXSMM_LOCK_INIT(internal_reglock + i);
      LIBXSMM_ATOMIC_STORE_ZERO(&internal_reglock_check, LIBXSMM_ATOMIC_SEQ_CST);
    }
  }
  while (0 != internal_reglock_check); /* wait until locks are initialized */
  for (i = 0; i < INTERNAL_REGLOCK_COUNT; ++i) LIBXSMM_LOCK_ACQUIRE(internal_reglock + i);
# endif
#else
# pragma omp critical(internal_reglock)
#endif
  {
    result = LIBXSMM_ATOMIC_LOAD(&internal_registry, LIBXSMM_ATOMIC_SEQ_CST);
    if (0 == result) {
      int init_code;
      /* set internal_target_archid */
      libxsmm_set_target_arch(getenv("LIBXSMM_TARGET"));
      { /* select prefetch strategy for JIT */
        const char *const env_prefetch = getenv("LIBXSMM_PREFETCH");
        if (0 == env_prefetch || 0 == *env_prefetch) {
#if (0 > LIBXSMM_PREFETCH) /* permitted by LIBXSMM_PREFETCH_AUTO */
          internal_prefetch = (LIBXSMM_X86_AVX512_MIC != internal_target_archid
            ? LIBXSMM_PREFETCH_NONE : LIBXSMM_PREFETCH_AL2BL2_VIA_C);
#else
          internal_prefetch = LIBXSMM_MAX(INTERNAL_PREFETCH, 0);
#endif
        }
        else { /* user input considered even if LIBXSMM_PREFETCH_AUTO is disabled */
          switch (atoi(env_prefetch)) {
            case 2:  internal_prefetch = LIBXSMM_PREFETCH_SIGONLY; break;
            case 3:  internal_prefetch = LIBXSMM_PREFETCH_BL2_VIA_C; break;
            case 4:  internal_prefetch = LIBXSMM_PREFETCH_AL2; break;
            case 5:  internal_prefetch = LIBXSMM_PREFETCH_AL2_AHEAD; break;
            case 6:  internal_prefetch = LIBXSMM_PREFETCH_AL2BL2_VIA_C; break;
            case 7:  internal_prefetch = LIBXSMM_PREFETCH_AL2BL2_VIA_C_AHEAD; break;
            case 8:  internal_prefetch = LIBXSMM_PREFETCH_AL2_JPST; break;
            case 9:  internal_prefetch = LIBXSMM_PREFETCH_AL2BL2_VIA_C_JPST; break;
            default: internal_prefetch = LIBXSMM_PREFETCH_NONE;
          }
        }
      }
      libxsmm_hash_init(internal_target_archid);
      libxsmm_gemm_diff_init(internal_target_archid);
      init_code = libxsmm_gemm_init(internal_target_archid, internal_prefetch);
#if defined(__TRACE)
      {
        int filter_threadid = 0, filter_mindepth = 1, filter_maxnsyms = 0;
        const char *const env_trace_init = getenv("LIBXSMM_TRACE");
        if (EXIT_SUCCESS == init_code && 0 != env_trace_init && 0 != *env_trace_init) {
          char buffer[32];
          if (1 == sscanf(env_trace_init, "%32[^,],", buffer)) {
            sscanf(buffer, "%i", &filter_threadid);
          }
          if (1 == sscanf(env_trace_init, "%*[^,],%32[^,],", buffer)) {
            sscanf(buffer, "%i", &filter_mindepth);
          }
          if (1 == sscanf(env_trace_init, "%*[^,],%*[^,],%32s", buffer)) {
            sscanf(buffer, "%i", &filter_maxnsyms);
          }
          else {
            filter_maxnsyms = -1; /* all */
          }
        }
        init_code = libxsmm_trace_init(filter_threadid - 1, filter_mindepth, filter_maxnsyms);
      }
#endif
      if (EXIT_SUCCESS == init_code) {
        assert(0 == internal_registry_keys && 0 == internal_registry); /* should never happen */
        result = (internal_code_type*)libxsmm_malloc(LIBXSMM_REGSIZE * sizeof(internal_code_type));
        internal_registry_keys = (internal_regkey_type*)libxsmm_malloc(LIBXSMM_REGSIZE * sizeof(internal_regkey_type));
        if (0 != result && 0 != internal_registry_keys) {
          const char *const env_verbose = getenv("LIBXSMM_VERBOSE");
          internal_statistic_mnk = (unsigned int)(pow((double)(LIBXSMM_MAX_MNK), 0.3333333333333333) + 0.5);
          internal_statistic_sml = 13; internal_statistic_med = 23;
          if (0 != env_verbose && 0 != *env_verbose) {
            internal_verbose_mode = atoi(env_verbose);
          }
#if !defined(NDEBUG)
          else {
            internal_verbose_mode = 1; /* quiet -> verbose */
          }
#endif
          for (i = 0; i < LIBXSMM_REGSIZE; ++i) result[i].pmm = 0;
          /* omit registering code if JIT is enabled and if an ISA extension is found
           * which is beyond the static code path used to compile the library
           */
#if defined(LIBXSMM_BUILD)
# if (0 != LIBXSMM_JIT) && !defined(__MIC__)
          if (LIBXSMM_STATIC_TARGET_ARCH <= internal_target_archid && LIBXSMM_X86_AVX > internal_target_archid)
# endif
          { /* opening a scope for eventually declaring variables */
            /* setup the dispatch table for the statically generated code */
#           include <libxsmm_dispatch.h>
          }
#endif
          atexit(libxsmm_finalize);
          LIBXSMM_ATOMIC_STORE(&internal_registry, result, LIBXSMM_ATOMIC_SEQ_CST);
        }
        else {
#if !defined(NDEBUG) && defined(__TRACE) /* library code is expected to be mute */
          fprintf(stderr, "LIBXSMM: failed to allocate code registry!\n");
#endif
          libxsmm_free(internal_registry_keys);
          libxsmm_free(result);
        }
      }
#if !defined(NDEBUG) && defined(__TRACE) /* library code is expected to be mute */
      else {
        fprintf(stderr, "LIBXSMM: failed to initialize sub-component (error #%i)!\n", init_code);
      }
#endif
    }
  }
#if !defined(LIBXSMM_OPENMP) && !defined(LIBXSMM_NOSYNC) /* release locks */
  for (i = 0; i < INTERNAL_REGLOCK_COUNT; ++i) LIBXSMM_LOCK_RELEASE(internal_reglock + i);
#endif
  assert(result);
  return result;
}


LIBXSMM_API_DEFINITION
#if defined(__GNUC__)
LIBXSMM_ATTRIBUTE(constructor)
#endif
void libxsmm_init(void)
{
  const void *const registry = LIBXSMM_ATOMIC_LOAD(&internal_registry, LIBXSMM_ATOMIC_RELAXED);
  if (0 == registry) {
    internal_init();
  }
}


LIBXSMM_API
#if defined(__GNUC__)
LIBXSMM_ATTRIBUTE(no_instrument_function)
#endif
void libxsmm_finalize(void);

LIBXSMM_API_DEFINITION
#if defined(__GNUC__)
LIBXSMM_ATTRIBUTE(destructor)
#endif
void libxsmm_finalize(void)
{
  internal_code_type* registry = LIBXSMM_ATOMIC_LOAD(&internal_registry, LIBXSMM_ATOMIC_SEQ_CST);

  if (0 != registry) {
    int i;
#if !defined(LIBXSMM_OPENMP)
# if !defined(LIBXSMM_NOSYNC)
    /* acquire locks and thereby shortcut lazy initialization later on */
    for (i = 0; i < INTERNAL_REGLOCK_COUNT; ++i) LIBXSMM_LOCK_ACQUIRE(internal_reglock + i);
# endif
#else
#   pragma omp critical(internal_reglock)
#endif
    {
      registry = internal_registry;

      if (0 != registry) {
        internal_regkey_type *const registry_keys = internal_registry_keys;
        const char *const target_arch = internal_get_target_arch(internal_target_archid);
        unsigned int heapmem = (LIBXSMM_REGSIZE) * (sizeof(internal_code_type) + sizeof(internal_regkey_type));

        /* serves as an id to invalidate the thread-local cache; never decremented */
        ++internal_teardown;
#if defined(__TRACE)
        i = libxsmm_trace_finalize();
# if !defined(NDEBUG) /* library code is expected to be mute */
        if (EXIT_SUCCESS != i) {
          fprintf(stderr, "LIBXSMM: failed to finalize trace (error #%i)!\n", i);
        }
# endif
#endif
        libxsmm_gemm_finalize();
        libxsmm_gemm_diff_finalize();
        libxsmm_hash_finalize();

        /* make internal registry globally unavailable */
        LIBXSMM_ATOMIC_STORE_ZERO(&internal_registry, LIBXSMM_ATOMIC_SEQ_CST);
        internal_registry_keys = 0;

        for (i = 0; i < LIBXSMM_REGSIZE; ++i) {
          internal_code_type code = registry[i];
          if (0 != code.pmm) {
            const libxsmm_gemm_descriptor *const desc = &registry_keys[i].descriptor;
            const unsigned long long kernel_size = LIBXSMM_MNK_SIZE(desc->m, desc->n, desc->k);
            const int precision = (0 == (LIBXSMM_GEMM_FLAG_F32PREC & desc->flags) ? 0 : 1);
            const unsigned int statistic_sml = internal_statistic_sml;
            int bucket = 2;
            assert((LIBXSMM_HASH_COLLISION | LIBXSMM_CODE_STATIC) != code.imm);
            if (LIBXSMM_MNK_SIZE(statistic_sml, statistic_sml, statistic_sml) >= kernel_size) {
              bucket = 0;
            }
            else {
              const unsigned int statistic_med = internal_statistic_med;
              if (LIBXSMM_MNK_SIZE(statistic_med, statistic_med, statistic_med) >= kernel_size) {
                bucket = 1;
              }
            }
            if (0 == (LIBXSMM_CODE_STATIC & code.imm)) { /* check for allocated/generated JIT-code */
              void* buffer = 0;
              size_t size = 0;
              code.imm &= ~LIBXSMM_HASH_COLLISION; /* clear collision flag */
              if (EXIT_SUCCESS == libxsmm_alloc_info(code.pmm, &size, 0/*flags*/, &buffer)) {
                libxsmm_deallocate(code.pmm);
                ++internal_statistic[precision][bucket].njit;
                heapmem += (unsigned int)(size + (((char*)code.pmm) - (char*)buffer));
              }
            }
            else {
              ++internal_statistic[precision][bucket].nsta;
            }
          }
        }
        if (0 != internal_verbose_mode) { /* print statistic on termination */
          LIBXSMM_FLOCK(stderr);
          LIBXSMM_FLOCK(stdout);
          fflush(stdout); /* synchronize with standard output */
          {
            const unsigned int linebreak = 0 == internal_print_statistic(stderr, target_arch, 1/*SP*/, 1, 0) ? 1 : 0;
            if (0 == internal_print_statistic(stderr, target_arch, 0/*DP*/, linebreak, 0) && 0 != linebreak) {
              fprintf(stderr, "LIBXSMM_TARGET=%s ", target_arch);
            }
            fprintf(stderr, "HEAP: %.1f MB\n", 1.0 * heapmem / (1 << 20));
          }
          LIBXSMM_FUNLOCK(stdout);
          LIBXSMM_FUNLOCK(stderr);
        }
        libxsmm_free(registry_keys);
        libxsmm_free(registry);
      }
    }
#if !defined(LIBXSMM_OPENMP) && !defined(LIBXSMM_NOSYNC) /* release locks */
    for (i = 0; i < INTERNAL_REGLOCK_COUNT; ++i) LIBXSMM_LOCK_RELEASE(internal_reglock + i);
#endif
  }
}


LIBXSMM_API_DEFINITION int libxsmm_get_target_archid(void)
{
  LIBXSMM_INIT
#if !defined(__MIC__) && (!defined(__CYGWIN__) || !defined(NDEBUG)/*code-coverage with Cygwin; fails@runtime!*/)
  return internal_target_archid;
#else /* no JIT support */
  return LIBXSMM_MIN(internal_target_archid, LIBXSMM_X86_SSE4_2);
#endif
}


LIBXSMM_API_DEFINITION void libxsmm_set_target_archid(int id)
{
  switch (id) {
    case LIBXSMM_X86_AVX512_CORE:
    case LIBXSMM_X86_AVX512_MIC:
    case LIBXSMM_X86_AVX2:
    case LIBXSMM_X86_AVX:
    case LIBXSMM_X86_SSE4_2:
    case LIBXSMM_X86_SSE4_1:
    case LIBXSMM_X86_SSE3:
    case LIBXSMM_TARGET_ARCH_GENERIC: {
      internal_target_archid = id;
    } break;
    default: if (LIBXSMM_X86_GENERIC <= id) {
      internal_target_archid = LIBXSMM_X86_GENERIC;
    }
    else {
      internal_target_archid = LIBXSMM_TARGET_ARCH_UNKNOWN;
    }
  }

#if !defined(NDEBUG) /* library code is expected to be mute */
  {
    const int target_archid = internal_target_archid;
    const int cpuid = libxsmm_cpuid_x86();
    if (cpuid < target_archid) {
      const char *const target_arch = internal_get_target_arch(target_archid);
      fprintf(stderr, "LIBXSMM: \"%s\" code will fail to run on \"%s\"!\n",
        target_arch, internal_get_target_arch(cpuid));
    }
  }
#endif
}


LIBXSMM_API_DEFINITION const char* libxsmm_get_target_arch(void)
{
  LIBXSMM_INIT
  return internal_get_target_arch(internal_target_archid);
}


/* function serves as a helper for implementing the Fortran interface */
LIBXSMM_API const char* get_target_arch(int* length);
LIBXSMM_API_DEFINITION const char* get_target_arch(int* length)
{
  const char *const arch = libxsmm_get_target_arch();
  /* valid here since function is not in the public interface */
  assert(0 != arch && 0 != length);
  *length = (int)strlen(arch);
  return arch;
}


LIBXSMM_API_DEFINITION void libxsmm_set_target_arch(const char* arch)
{
  int target_archid = LIBXSMM_TARGET_ARCH_UNKNOWN;
  if (0 != arch && 0 != *arch) {
    const int jit = atoi(arch);
    if (0 == strcmp("0", arch)) {
      target_archid = LIBXSMM_TARGET_ARCH_GENERIC;
    }
    else if (1 < jit) {
      target_archid = LIBXSMM_X86_GENERIC + jit;
    }
    else if (0 == strcmp("skx", arch) || 0 == strcmp("avx3", arch) || 0 == strcmp("avx512", arch)) {
      target_archid = LIBXSMM_X86_AVX512_CORE;
    }
    else if (0 == strcmp("knl", arch) || 0 == strcmp("mic2", arch)) {
      target_archid = LIBXSMM_X86_AVX512_MIC;
    }
    else if (0 == strcmp("hsw", arch) || 0 == strcmp("avx2", arch)) {
      target_archid = LIBXSMM_X86_AVX2;
    }
    else if (0 == strcmp("snb", arch) || 0 == strcmp("avx", arch)) {
      target_archid = LIBXSMM_X86_AVX;
    }
    else if (0 == strcmp("wsm", arch) || 0 == strcmp("nhm", arch) || 0 == strcmp("sse4", arch) || 0 == strcmp("sse4_2", arch) || 0 == strcmp("sse4.2", arch)) {
      target_archid = LIBXSMM_X86_SSE4_2;
    }
    else if (0 == strcmp("sse4_1", arch) || 0 == strcmp("sse4.1", arch)) {
      target_archid = LIBXSMM_X86_SSE4_1;
    }
    else if (0 == strcmp("sse3", arch) || 0 == strcmp("sse", arch)) {
      target_archid = LIBXSMM_X86_SSE3;
    }
    else if (0 == strcmp("x86", arch) || 0 == strcmp("sse2", arch)) {
      target_archid = LIBXSMM_X86_GENERIC;
    }
    else if (0 == strcmp("generic", arch) || 0 == strcmp("none", arch)) {
      target_archid = LIBXSMM_TARGET_ARCH_GENERIC;
    }
  }

  if (LIBXSMM_TARGET_ARCH_UNKNOWN == target_archid || LIBXSMM_X86_AVX512_CORE < target_archid) {
    target_archid = libxsmm_cpuid_x86();
  }
#if !defined(NDEBUG) /* library code is expected to be mute */
  else {
    const int cpuid = libxsmm_cpuid_x86();
    if (cpuid < target_archid) {
      const char *const target_arch = internal_get_target_arch(target_archid);
      fprintf(stderr, "LIBXSMM: \"%s\" code will fail to run on \"%s\"!\n",
        target_arch, internal_get_target_arch(cpuid));
    }
  }
#endif
  internal_target_archid = target_archid;
}


LIBXSMM_API_DEFINITION int libxsmm_get_verbose_mode(void)
{
  LIBXSMM_INIT
  return internal_verbose_mode;
}


LIBXSMM_API_DEFINITION void libxsmm_set_verbose_mode(int mode)
{
  LIBXSMM_INIT
  internal_verbose_mode = mode;
}


LIBXSMM_INLINE LIBXSMM_RETARGETABLE void internal_build(const libxsmm_gemm_descriptor* descriptor,
  const char* jit_kind, const internal_desc_extra_type* desc_extra, internal_code_type* code)
{
#if !defined(__MIC__) && (!defined(__CYGWIN__) || !defined(NDEBUG)/*code-coverage with Cygwin; fails@runtime!*/)
  const char *const target_arch = internal_get_target_arch(internal_target_archid);
  libxsmm_generated_code generated_code;
  assert(0 != descriptor && 0 < descriptor->m && 0 < descriptor->n && 0 < descriptor->k);
  assert(0 < descriptor->lda && 0 < descriptor->lda && 0 < descriptor->ldc);
  assert(0 != internal_target_archid);
  assert(0 != code && 0 == code->pmm);

  /* allocate temporary buffer which is large enough to cover the generated code */
  generated_code.generated_code = malloc(131072);
  generated_code.buffer_size = 0 != generated_code.generated_code ? 131072 : 0;
  generated_code.code_size = 0;
  generated_code.code_type = 2;
  generated_code.last_error = 0;

  /* generate kernel */
  if (0 == desc_extra) {
    libxsmm_generator_gemm_kernel(&generated_code, descriptor, target_arch);
  }
  else if (0 != desc_extra->row_ptr && 0 != desc_extra->column_idx &&
    0 != desc_extra->values)
  { /* currently only one additional kernel kind */
    assert(0 == (LIBXSMM_GEMM_FLAG_F32PREC & (descriptor->flags)));
    libxsmm_generator_spgemm_csr_soa_kernel(&generated_code, descriptor, target_arch,
      desc_extra->row_ptr, desc_extra->column_idx, (const double*)desc_extra->values);
  }

  /* handle an eventual error in the else-branch */
  if (0 == generated_code.last_error) {
    /* attempt to create executable buffer, and check for success */
    if (0 == libxsmm_allocate(&code->pmm, generated_code.code_size, 0/*auto*/,
      /* must be a superset of what mprotect populates (see below) */
      LIBXSMM_ALLOC_FLAG_RWX,
      0/*extra*/, 0/*extra_size*/))
    {
      char jit_code_name[256];
# if !defined(LIBXSMM_VTUNE)
      LIBXSMM_UNUSED(jit_kind);
      if (0 > internal_verbose_mode)
# endif
      {
        internal_get_code_name(target_arch, jit_kind, descriptor, sizeof(jit_code_name), jit_code_name);
      }
# if !defined(LIBXSMM_VTUNE)
      else {
        jit_code_name[0] = 0;
      }
# endif
      assert(0 == ((LIBXSMM_HASH_COLLISION | LIBXSMM_CODE_STATIC) & code->imm));
      /* copy temporary buffer into the prepared executable buffer */
      memcpy(code->pmm, generated_code.generated_code, generated_code.code_size);
      /* revoke unnecessary memory protection flags; continue on error */
      libxsmm_alloc_attribute(code->pmm, LIBXSMM_ALLOC_FLAG_RW, jit_code_name);
    }
  }
# if !defined(NDEBUG) /* library code is expected to be mute */
  else {
    static LIBXSMM_TLS int error_jit = 0;
    if (0 == error_jit) {
      fprintf(stderr, "%s (error #%u)\n", libxsmm_strerror(generated_code.last_error),
        generated_code.last_error);
      error_jit = 1;
    }
  }
# endif
  free(generated_code.generated_code); /* free temporary/initial code buffer */
#else /* unsupported platform */
# if !defined(__MIC__)
  LIBXSMM_MESSAGE("================================================================================")
  LIBXSMM_MESSAGE("LIBXSMM: The JIT BACKEND is currently not supported under Microsoft Windows!")
  LIBXSMM_MESSAGE("================================================================================")
# endif
  LIBXSMM_UNUSED(descriptor); LIBXSMM_UNUSED(jit_kind); LIBXSMM_UNUSED(desc_extra);  LIBXSMM_UNUSED(code);
  /* libxsmm_get_target_arch also serves as a runtime check whether JIT is available or not */
  assert(LIBXSMM_X86_AVX > internal_target_archid);
#endif
}


LIBXSMM_API_DEFINITION libxsmm_xmmfunction libxsmm_xmmdispatch(const libxsmm_gemm_descriptor* descriptor)
{
  const libxsmm_xmmfunction null_mmfunction = { 0 };
  LIBXSMM_INIT
  if (0 != descriptor && INTERNAL_DISPATCH_BYPASS_CHECK(descriptor->flags, descriptor->alpha, descriptor->beta)) {
    libxsmm_gemm_descriptor backend_descriptor;
    if (0 > (int)descriptor->prefetch) {
      backend_descriptor = *descriptor;
      backend_descriptor.prefetch = (unsigned char)internal_prefetch;
      descriptor = &backend_descriptor;
    }
    {
      INTERNAL_FIND_CODE_DECLARE(code);
      INTERNAL_FIND_CODE(descriptor, code);
    }
  }
  else { /* TODO: not supported (bypass) */
    return null_mmfunction;
  }
}


LIBXSMM_API_DEFINITION libxsmm_smmfunction libxsmm_smmdispatch(int m, int n, int k,
  const int* lda, const int* ldb, const int* ldc,
  const float* alpha, const float* beta,
  const int* flags, const int* prefetch)
{
  LIBXSMM_INIT
  INTERNAL_SMMDISPATCH(flags, m, n, k, lda, ldb, ldc, alpha, beta, prefetch);
}


LIBXSMM_API_DEFINITION libxsmm_dmmfunction libxsmm_dmmdispatch(int m, int n, int k,
  const int* lda, const int* ldb, const int* ldc,
  const double* alpha, const double* beta,
  const int* flags, const int* prefetch)
{
  LIBXSMM_INIT
  INTERNAL_DMMDISPATCH(flags, m, n, k, lda, ldb, ldc, alpha, beta, prefetch);
}


LIBXSMM_API_DEFINITION libxsmm_xmmfunction libxsmm_create_dcsr_soa(const libxsmm_gemm_descriptor* descriptor,
  const unsigned int* row_ptr, const unsigned int* column_idx, const double* values)
{
  internal_code_type code = { {0} };
  internal_desc_extra_type desc_extra;
  LIBXSMM_INIT
  memset(&desc_extra, 0, sizeof(desc_extra));
  desc_extra.row_ptr = row_ptr;
  desc_extra.column_idx = column_idx;
  desc_extra.values = values;
  internal_build(descriptor, "csr", &desc_extra, &code);
  return code.xmm;
}


LIBXSMM_API_DEFINITION void libxsmm_destroy(const void* jit_code)
{
  LIBXSMM_INIT
  libxsmm_deallocate(jit_code);
}

