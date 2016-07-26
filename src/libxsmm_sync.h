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
/* Hans Pabst (Intel Corp.)
******************************************************************************/
#ifndef LIBXSMM_SYNC_H
#define LIBXSMM_SYNC_H

#if defined(LIBXSMM_NOSYNC)
# undef _REENTRANT
#elif !defined(_REENTRANT)
# define _REENTRANT
#endif

#if defined(_REENTRANT)
# if (defined(_WIN32) && !defined(__GNUC__))
#   define LIBXSMM_TLS LIBXSMM_ATTRIBUTE(thread)
# elif defined(__GNUC__)
#   define LIBXSMM_TLS __thread
# elif defined(__cplusplus)
#   define LIBXSMM_TLS thread_local
# else
#   error Missing TLS support!
# endif
#else
# define LIBXSMM_TLS
#endif

#if defined(__GNUC__)
# if !defined(LIBXSMM_GCCATOMICS)
#   if (LIBXSMM_VERSION3(4, 7, 4) <= LIBXSMM_VERSION3(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__))
#     define LIBXSMM_GCCATOMICS 1
#   else
#     define LIBXSMM_GCCATOMICS 0
#   endif
# endif
#endif

#define LIBXSMM_ATOMIC_RELAXED __ATOMIC_RELAXED
#define LIBXSMM_ATOMIC_SEQ_CST __ATOMIC_SEQ_CST

#if (defined(_REENTRANT) || defined(LIBXSMM_OPENMP)) && defined(LIBXSMM_GCCATOMICS)
# if (0 != LIBXSMM_GCCATOMICS)
#   define LIBXSMM_ATOMIC_LOAD(SRC_PTR, KIND) __atomic_load_n(SRC_PTR, KIND)
#   define LIBXSMM_ATOMIC_STORE(DST_PTR, VALUE, KIND) __atomic_store_n(DST_PTR, VALUE, KIND)
#   define LIBXSMM_ATOMIC_ADD_FETCH(DST_PTR, VALUE, KIND) /**(DST_PTR) =*/ __atomic_add_fetch(DST_PTR, VALUE, KIND)
# else
#   define LIBXSMM_ATOMIC_LOAD(SRC_PTR, KIND) __sync_or_and_fetch(SRC_PTR, 0)
#   define LIBXSMM_ATOMIC_STORE(DST_PTR, VALUE, KIND) *(DST_PTR) = VALUE; \
      while (0/*false*/ == __sync_bool_compare_and_swap(DST_PTR, VALUE, VALUE))
    /* use store side-effect of built-in (dummy assignment to mute warning) */
#   define LIBXSMM_ATOMIC_STORE_ZERO(DST_PTR, KIND) __sync_and_and_fetch(DST_PTR, 0)
#   define LIBXSMM_ATOMIC_ADD_FETCH(DST_PTR, VALUE, KIND) /**(DST_PTR) = */__sync_add_and_fetch(DST_PTR, VALUE)
# endif
#elif (defined(_REENTRANT) || defined(LIBXSMM_OPENMP)) && defined(_WIN32) /*TODO*/
#   define LIBXSMM_ATOMIC_LOAD(SRC_PTR, KIND) *(SRC_PTR)
#   define LIBXSMM_ATOMIC_STORE(DST_PTR, VALUE, KIND) *(DST_PTR) = VALUE
#   define LIBXSMM_ATOMIC_ADD_FETCH(DST_PTR, VALUE, KIND) *(DST_PTR) += VALUE
#else
#   define LIBXSMM_ATOMIC_LOAD(SRC_PTR, KIND) *(SRC_PTR)
#   define LIBXSMM_ATOMIC_STORE(DST_PTR, VALUE, KIND) *(DST_PTR) = VALUE
#   define LIBXSMM_ATOMIC_ADD_FETCH(DST_PTR, VALUE, KIND) *(DST_PTR) += VALUE
#endif

#if !defined(LIBXSMM_ATOMIC_STORE_ZERO)
# define LIBXSMM_ATOMIC_STORE_ZERO(DST_PTR, KIND) LIBXSMM_ATOMIC_STORE(DST_PTR, 0, KIND)
#endif

#if defined(_WIN32) /*TODO*/
# define LIBXSMM_LOCK_ACQUIRED WAIT_OBJECT_0
# define LIBXSMM_LOCK_TYPE HANDLE
# define LIBXSMM_LOCK_CONSTRUCT 0
# define LIBXSMM_LOCK_INIT(LOCK) /*TODO*/
# define LIBXSMM_LOCK_DESTROY(LOCK) CloseHandle(LOCK)
# define LIBXSMM_LOCK_ACQUIRE(LOCK) WaitForSingleObject(LOCK, INFINITE)
# define LIBXSMM_LOCK_TRYLOCK(LOCK) WaitForSingleObject(LOCK, 0)
# define LIBXSMM_LOCK_RELEASE(LOCK) ReleaseMutex(LOCK)
#else /* PThreads: include <pthread.h> */
# define LIBXSMM_LOCK_ACQUIRED 0
# define LIBXSMM_LOCK_TYPE pthread_mutex_t
# define LIBXSMM_LOCK_CONSTRUCT PTHREAD_MUTEX_INITIALIZER
# define LIBXSMM_LOCK_INIT(LOCK) pthread_mutex_init(LOCK, 0)
# define LIBXSMM_LOCK_DESTROY(LOCK) pthread_mutex_destroy(LOCK)
# define LIBXSMM_LOCK_ACQUIRE(LOCK) pthread_mutex_lock(LOCK)
# define LIBXSMM_LOCK_TRYLOCK(LOCK) pthread_mutex_trylock(LOCK)
# define LIBXSMM_LOCK_RELEASE(LOCK) pthread_mutex_unlock(LOCK)
#endif

#endif /*LIBXSMM_SYNC_H*/

