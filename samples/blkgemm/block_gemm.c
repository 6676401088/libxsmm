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
/* Alexander Heinecke (Intel Corp.), Hans Pabst (Intel Corp.)
******************************************************************************/

#include <libxsmm.h>
#include <libxsmm_timer.h>
#include <libxsmm_malloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/** Function prototype for SGEMM; this way any kind of LAPACK/BLAS library is sufficient at link-time. */
void LIBXSMM_FSYMBOL(sgemm)(const char*, const char*, const libxsmm_blasint*, const libxsmm_blasint*, const libxsmm_blasint*,
  const float*, const float*, const libxsmm_blasint*, const float*, const libxsmm_blasint*,
  const float*, float*, const libxsmm_blasint*);
/** Function prototype for DGEMM; this way any kind of LAPACK/BLAS library is sufficient at link-time. */
void LIBXSMM_FSYMBOL(dgemm)(const char*, const char*, const libxsmm_blasint*, const libxsmm_blasint*, const libxsmm_blasint*,
  const double*, const double*, const libxsmm_blasint*, const double*, const libxsmm_blasint*,
  const double*, double*, const libxsmm_blasint*);

typedef float real;

typedef struct libxsmm_blkgemm_handle {
  int m;
  int n;
  int k;
  int bm;
  int bn;
  int bk;
  int mb;
  int nb;
  int kb;
  libxsmm_smmfunction kernel;
} libxsmm_blkgemm_handle;

void libxsmm_blksgemm_init_a( libxsmm_blkgemm_handle* handle,
                              float* libxsmm_mat_dst,
                              float* colmaj_mat_src ) {
  int mb, kb, bm, bk;
#if defined(LIBXSMM_VLA)
  typedef float (*LIBXSMM_RESTRICT dst_type)[handle->mb][handle->bk][handle->bm];
  typedef float (*LIBXSMM_RESTRICT src_type)[handle->m];
  const dst_type dst = (dst_type)libxsmm_mat_dst;
  const src_type src = (src_type)colmaj_mat_src;

  for ( kb = 0; kb < handle->kb; kb++ ) {
    for ( mb = 0; mb < handle->mb; mb++ ) {
      for ( bk = 0; bk < handle->bk; bk++ ) {
        for ( bm = 0; bm < handle->bm; bm++ ) {
          dst[kb][mb][bk][bm] = src[(kb*handle->bk)+bk][(mb*handle->bm)+bm];
        }
      }
    }
  }
#else
#error this code only works with LIBXSMM_VLA being available.
#endif
}

void libxsmm_blksgemm_init_b( libxsmm_blkgemm_handle* handle,
                              float* libxsmm_mat_dst,
                              float* colmaj_mat_src ) {
  int kb, nb, bk, bn;
#if defined(LIBXSMM_VLA)
  typedef float (*LIBXSMM_RESTRICT dst_type)[handle->kb][handle->bn][handle->bk];
  typedef float (*LIBXSMM_RESTRICT src_type)[handle->k];
  const dst_type dst = (dst_type)libxsmm_mat_dst;
  const src_type src = (src_type)colmaj_mat_src;

  for ( nb = 0; nb < handle->nb; nb++ ) {
    for ( kb = 0; kb < handle->kb; kb++ ) {
      for ( bn = 0; bn < handle->bn; bn++ ) {
        for ( bk = 0; bk < handle->bk; bk++ ) {
          dst[nb][kb][bn][bk] = src[(nb*handle->bn)+bn][(kb*handle->bk)+bk];
        }
      }
    }
  }
#else
#error this code only works with LIBXSMM_VLA being available.
#endif
}

void libxsmm_blksgemm_init_c( libxsmm_blkgemm_handle* handle,
                              float* libxsmm_mat_dst,
                              float* colmaj_mat_src ) {
  int mb, nb, bm, bn;
#if defined(LIBXSMM_VLA)
  typedef float (*LIBXSMM_RESTRICT dst_type)[handle->mb][handle->bn][handle->bm];
  typedef float (*LIBXSMM_RESTRICT src_type)[handle->m];
  const dst_type dst = (dst_type)libxsmm_mat_dst;
  const src_type src = (src_type)colmaj_mat_src;

  for ( nb = 0; nb < handle->nb; nb++ ) {
    for ( mb = 0; mb < handle->mb; mb++ ) {
      for ( bn = 0; bn < handle->bn; bn++ ) {
        for ( bm = 0; bm < handle->bm; bm++ ) {
          dst[nb][mb][bn][bm] = src[(nb*handle->bn)+bn][(mb*handle->bm)+bm];
        }
      }
    }
  }
#else
#error this code only works with LIBXSMM_VLA being available.
#endif
}

void libxsmm_blksgemm_check_c( libxsmm_blkgemm_handle* handle,
                               real* libxsmm_mat_dst,
                               real* colmaj_mat_src ) {
  int mb, nb, bm, bn;
  double max_error = 0.0;
  double src_norm = 0.0;
  double dst_norm = 0.0;
#if defined(LIBXSMM_VLA)
  typedef float (*LIBXSMM_RESTRICT dst_type)[handle->mb][handle->bn][handle->bm];
  typedef float (*LIBXSMM_RESTRICT src_type)[handle->m];
  const dst_type dst = (dst_type)libxsmm_mat_dst;
  const src_type src = (src_type)colmaj_mat_src;

  for ( nb = 0; nb < handle->nb; nb++ ) {
    for ( mb = 0; mb < handle->mb; mb++ ) {
      for ( bn = 0; bn < handle->bn; bn++ ) {
        for ( bm = 0; bm < handle->bm; bm++ ) {
          double local_error = fabs((double)dst[nb][mb][bn][bm] - (double)src[(nb*handle->bn)+bn][(mb*handle->bm)+bm]);
          if (local_error > max_error) {
            max_error = local_error;
          }
          src_norm += (double)src[(nb*handle->bn)+bn][(mb*handle->bm)+bm];
          dst_norm += (double)dst[nb][mb][bn][bm];
        }
      }
    }
  }
#else
#error this code only works with LIBXSMM_VLA being available.
#endif
  printf(" max error: %f, sum BLAS: %f, sum LIBXSMM: %f \n", max_error, src_norm, dst_norm );
}

void libxsmm_blksgemm_exec( const libxsmm_blkgemm_handle* handle, 
                            const char transA, 
                            const char transB, 
                            const float* alpha, 
                            const float* a, 
                            const float* b, 
                            const float* beta, 
                            float* c ) {
  int mb, nb, kb, mb2, nb2, kb2;
#if defined(LIBXSMM_VLA)
  typedef float (*LIBXSMM_RESTRICT a_type)[handle->mb][handle->bk][handle->bm];
  typedef float (*LIBXSMM_RESTRICT b_type)[handle->kb][handle->bn][handle->bk];
  typedef float (*LIBXSMM_RESTRICT c_type)[handle->mb][handle->bn][handle->bm];

  const a_type a_t = (a_type)a;
  const b_type b_t = (b_type)b;
  const c_type c_t = (c_type)c;

  int mr = 8;
  int nr = 8;
  int kr = 4;

  if ( (handle->mb % mr == 0) && (handle->nb % nr == 0) && (handle->kb % kr == 0) ) {
#if 0
    #pragma omp parallel for collapse(2) private(mb, nb, kb, mb2, nb2, kb2)
#endif
    for ( nb = 0; nb < handle->nb; nb+=nr ) {
      for ( mb = 0; mb < handle->mb; mb+=mr ) {
        for ( kb = 0; kb < handle->kb; kb+=kr ) {
          for ( nb2 = nb; nb2 < nb+nr; nb2++ ) {
            for ( mb2 = mb; mb2 < mb+mr; mb2++ ) {
              for ( kb2 = kb; kb2 < kb+kr; kb2++ ) {
                handle->kernel( &(a_t[kb2][mb2][0][0]), &(b_t[nb2][kb2][0][0]), &(c_t[nb2][mb2][0][0]) );
              }
            }
          }
        }
      }
    }
  } else {
#if 0
    #pragma omp parallel for collapse(2) private(mb, nb, kb)
#endif
    for ( nb = 0; nb < handle->nb; nb++ ) {
      for ( mb = 0; mb < handle->mb; mb++ ) {
        for ( kb = 0; kb < handle->kb; kb++ ) {
          handle->kernel( &(a_t[kb][mb][0][0]), &(b_t[nb][kb][0][0]), &(c_t[nb][mb][0][0]) );
        }
      }
    }
  }
#else
#error this code only works with LIBXSMM_VLA being available.
#endif
}

int main(int argc, char* argv []) {
  real *a, *b, *c, *a_gold, *b_gold, *c_gold;
  int M, N, K, LDA, LDB, LDC;
  real alpha, beta;
  unsigned long long start, end;
  double total, flops;
  int i, reps;
  size_t l;
  char trans;
  libxsmm_blkgemm_handle handle;

  /* init */
/*
  a = 0;
  b = 0;
  c = 0;
  a_gold = 0;
  b_gold = 0;
  c_gold = 0;
*/
  M = 0;
  N = 0;
  K = 0;
  LDA = 0;
  LDB = 0;
  LDC = 0;
  alpha = (real)1.0;
  beta = (real)1.0;
  start = 0;
  end = 0;
  total = 0.0;
  flops = 0.0;
  i = 0;
  l = 0;
  reps = 0;
  trans = 'N';

  /* check command line */
  if (argc > 1 && !strncmp(argv[1], "-h", 3)) {
    printf("\nUsage: ./block_gemm [M] [N] [K] [bm] [bn] [bk] [reps]\n\n");
    return 0;
  }

  /* setup defaults */
  handle.m = 2048;
  handle.n = 2048;
  handle.k = 2048;
  handle.bm = 32;
  handle.bn = 32;
  handle.bk = 32;
  reps = 100;

  /* reading new values from cli */
  i = 1;
  if (argc > i) handle.m      = atoi(argv[i++]);
  if (argc > i) handle.n      = atoi(argv[i++]);
  if (argc > i) handle.k      = atoi(argv[i++]);
  if (argc > i) handle.bm     = atoi(argv[i++]);
  if (argc > i) handle.bn     = atoi(argv[i++]);
  if (argc > i) handle.bk     = atoi(argv[i++]);
  if (argc > i) reps          = atoi(argv[i++]);
  M = handle.m;
  LDA = handle.m;
  N = handle.n;
  LDB = handle.k;
  K = handle.k;
  LDC = handle.m;
  alpha = (real)1.0;
  beta = (real)1.0;
  flops = (double)M * (double)N * (double)K * (double)2.0 * (double)reps;

  /* check for valide blocking and JIT-kernel */
  if ( handle.m % handle.bm != 0 ) {
    printf( " M needs to be a multiple of bm... exiting!\n" );
    return -1;
  }
  if ( handle.n % handle.bn != 0 ) {
    printf( " N needs to be a multiple of bn... exiting!\n" );
    return -2;
  }
  if ( handle.k % handle.bk != 0 ) {
    printf( " K needs to be a multiple of bk... exiting!\n" );
    return -3;
  }
  handle.mb = handle.m / handle.bm;
  handle.nb = handle.n / handle.bn;
  handle.kb = handle.k / handle.bk;
  handle.kernel = libxsmm_smmdispatch(handle.bm, handle.bn, handle.bk, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

  /* init random seed and print some info */
  printf(" Running with: M=%i, N=%i, K=%i, bm=%i, bn=%i, bk=%i, reps=%i\n", M, N, K, handle.bm, handle.bn, handle.bk, reps );
  srand48(1);

  /* allocate data */
  a      = (real*)libxsmm_aligned_malloc( M*K*sizeof(real), 2097152 );
  b      = (real*)libxsmm_aligned_malloc( K*N*sizeof(real), 2097152 );
  c      = (real*)libxsmm_aligned_malloc( M*N*sizeof(real), 2097152 );
  a_gold = (real*)libxsmm_aligned_malloc( M*K*sizeof(real), 2097152 );
  b_gold = (real*)libxsmm_aligned_malloc( K*N*sizeof(real), 2097152 );
  c_gold = (real*)libxsmm_aligned_malloc( M*N*sizeof(real), 2097152 );

  /* init data */
  for ( l = 0; l < (size_t)M * (size_t)K; l++ ) {
    a_gold[l] = (real)drand48();
  }
  for ( l = 0; l < (size_t)K * (size_t)N; l++ ) {
    b_gold[l] = (real)drand48();
  }
  for ( l = 0; l < (size_t)M * (size_t)N; l++ ) {
    c_gold[l] = (real)0.0;
  }
  for ( l = 0; l < (size_t)M * (size_t)N; l++ ) {
    c[l]      = (real)0.0;
  }
  libxsmm_blksgemm_init_a( &handle, a, a_gold );
  libxsmm_blksgemm_init_b( &handle, b, b_gold );

  /* check result */
  /* run LIBXSEMM, trans, alpha and beta are ignored */
  libxsmm_blksgemm_exec( &handle, trans, trans, &alpha, a, b, &beta, c );
  /* run BLAS */
  LIBXSMM_FSYMBOL(sgemm)(&trans, &trans, &M, &N, &K, &alpha, a_gold, &LDA, b_gold, &LDB, &beta, c_gold, &LDC);
  /* compare result */
  libxsmm_blksgemm_check_c( &handle, c, c_gold );

  /* time BLAS */
  start = libxsmm_timer_tick();
  for ( i = 0; i < reps; i++ ) {
    LIBXSMM_FSYMBOL(sgemm)(&trans, &trans, &M, &N, &K, &alpha, a_gold, &LDA, b_gold, &LDB, &beta, c_gold, &LDC);
  }
  end = libxsmm_timer_tick();
  total = libxsmm_timer_duration(start, end);
  printf("GFLOPS  (BLAS)    = %.5g\n", (flops*1e-9)/total);

  /* time libxsmm */
  start = libxsmm_timer_tick();
  for ( i = 0; i < reps; i++ ) {
    libxsmm_blksgemm_exec( &handle, trans, trans, &alpha, a, b, &beta, c );
  }
  end = libxsmm_timer_tick();
  total = libxsmm_timer_duration(start, end);
  printf("GFLOPS  (LIBXSMM) = %.5g\n", (flops*1e-9)/total);

  /* free data */
  libxsmm_free( a );
  libxsmm_free( b );
  libxsmm_free( c );
  libxsmm_free( a_gold );
  libxsmm_free( b_gold );
  libxsmm_free( c_gold );

  return 0;
}

