!*****************************************************************************!
!* Copyright (c) 2013-2015, Intel Corporation                                *!
!* All rights reserved.                                                      *!
!*                                                                           *!
!* Redistribution and use in source and binary forms, with or without        *!
!* modification, are permitted provided that the following conditions        *!
!* are met:                                                                  *!
!* 1. Redistributions of source code must retain the above copyright         *!
!*    notice, this list of conditions and the following disclaimer.          *!
!* 2. Redistributions in binary form must reproduce the above copyright      *!
!*    notice, this list of conditions and the following disclaimer in the    *!
!*    documentation and/or other materials provided with the distribution.   *!
!* 3. Neither the name of the copyright holder nor the names of its          *!
!*    contributors may be used to endorse or promote products derived        *!
!*    from this software without specific prior written permission.          *!
!*                                                                           *!
!* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *!
!* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *!
!* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     *!
!* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      *!
!* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    *!
!* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  *!
!* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    *!
!* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    *!
!* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      *!
!* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        *!
!* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              *!
!*****************************************************************************!
!* Hans Pabst (Intel Corp.), Alexander Heinecke (Intel Corp.)                *!
!*****************************************************************************!

      MODULE LIBXSMM
        USE, INTRINSIC :: ISO_C_BINDING
        IMPLICIT NONE

        ! Kind of types used to parameterize the implementation.
        INTEGER, PARAMETER :: LIBXSMM_SINGLE_PRECISION  = 4 !KIND(1.0)
        INTEGER, PARAMETER :: LIBXSMM_DOUBLE_PRECISION  = 8 !KIND(1D0)
        INTEGER, PARAMETER :: LIBXSMM_INTEGER_TYPE      = KIND(1)

        ! Parameters the library was built for.
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_ALIGNMENT = $ALIGNMENT
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_ALIGNED_STORES = $ALIGNED_STORES
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_ALIGNED_LOADS = $ALIGNED_LOADS
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_ALIGNED_MAX = $ALIGNED_MAX
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_PREFETCH = $PREFETCH
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_ROW_MAJOR = $ROW_MAJOR
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_COL_MAJOR = $COL_MAJOR
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_MAX_MNK = $MAX_MNK
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_MAX_M = $MAX_M
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_MAX_N = $MAX_N
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_MAX_K = $MAX_K
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_AVG_M = $AVG_M
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_AVG_N = $AVG_N
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_AVG_K = $AVG_K
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_JIT = $JIT

        ! Parameters representing the GEMM performed by the simplified interface.
        REAL(LIBXSMM_DOUBLE_PRECISION), PARAMETER ::                    &
     &    LIBXSMM_ALPHA = $ALPHA
        REAL(LIBXSMM_DOUBLE_PRECISION), PARAMETER ::                    &
     &    LIBXSMM_BETA = $BETA

        ! Flag enumeration which can be IORed.
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_GEMM_FLAG_TRANS_A = 1,                                &
     &    LIBXSMM_GEMM_FLAG_TRANS_B = 2,                                &
     &    LIBXSMM_GEMM_FLAG_ALIGN_A = 4,                                &
     &    LIBXSMM_GEMM_FLAG_ALIGN_C = 8

        ! Flag representing the GEMM performed by the simplified interface.
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_GEMM_FLAG_DEFAULT = IOR(                              &
     &        MERGE(LIBXSMM_GEMM_FLAG_ALIGN_A, 0,                       &
     &          1.LT.LIBXSMM_ALIGNED_LOADS),                            &
     &        MERGE(LIBXSMM_GEMM_FLAG_ALIGN_C, 0,                       &
     &          1.LT.LIBXSMM_ALIGNED_STORES))

        ! Enumeration of the available prefetch strategies which can be IORed.
        !   LIBXSMM_PREFETCH_NONE:      No prefetching and no prefetch fn. signature.
        !   LIBXSMM_PREFETCH_SIGNATURE: Only function prefetch signature.
        !   LIBXSMM_PREFETCH_AL2:       Prefetch PA using accesses to A.
        !   LIBXSMM_PREFETCH_AL2_JPST:  Prefetch PA (aggressive).
        !   LIBXSMM_PREFETCH_BL2_VIA_C: Prefetch PB using accesses to C.
        !   LIBXSMM_PREFETCH_AL2_AHEAD: Prefetch A ahead.
        INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                     &
     &    LIBXSMM_PREFETCH_NONE       = 0,                              &
     &    LIBXSMM_PREFETCH_SIGNATURE  = 1,                              &
     &    LIBXSMM_PREFETCH_AL2        = 2,                              &
     &    LIBXSMM_PREFETCH_AL2_JPST   = 4,                              &
     &    LIBXSMM_PREFETCH_BL2_VIA_C  = 8,                              &
     &    LIBXSMM_PREFETCH_AL2_AHEAD  = 16,                             &
          ! Composed prefetch strategies.
     &    LIBXSMM_PREFETCH_AL2BL2_VIA_C = IOR(                          &
     &        LIBXSMM_PREFETCH_BL2_VIA_C, LIBXSMM_PREFETCH_AL2),        &
     &    LIBXSMM_PREFETCH_AL2BL2_VIA_C_JPST = IOR(                     &
     &        LIBXSMM_PREFETCH_BL2_VIA_C, LIBXSMM_PREFETCH_AL2_JPST),   &
     &    LIBXSMM_PREFETCH_AL2BL2_VIA_C_AHEAD = IOR(                    &
     &        LIBXSMM_PREFETCH_BL2_VIA_C, LIBXSMM_PREFETCH_AL2_AHEAD)

        ! Structure providing the actual/extended arguments of an SGEMM call.
        TYPE, BIND(C) :: LIBXSMM_SGEMM_XARGS
          ! The Alpha and Beta arguments.
          REAL(C_FLOAT) :: alpha, beta
          ! The prefetch arguments.
          TYPE(C_PTR) :: pa, pb, pc
        END TYPE
        ! Constructs an actual/extended argument set for an SGEMM call.
        INTERFACE LIBXSMM_SGEMM_XARGS_INTERFACE
          MODULE PROCEDURE LIBXSMM_SGEMM_XARGS_CTOR
        END INTERFACE

        ! Structure providing the actual/extended arguments of a DGEMM call.
        TYPE, BIND(C) :: LIBXSMM_DGEMM_XARGS
          ! The Alpha and Beta arguments.
          REAL(C_DOUBLE) :: alpha, beta
          ! The prefetch arguments.
          TYPE(C_PTR) :: pa, pb, pc
        END TYPE
        ! Constructs an actual/extended argument set for an SGEMM call.
        INTERFACE LIBXSMM_DGEMM_XARGS_INTERFACE
          MODULE PROCEDURE LIBXSMM_DGEMM_XARGS_CTOR
        END INTERFACE

        ! Overloaded dispatch/JIT routines (single/double precision).
        INTERFACE libxsmm_dispatch
          MODULE PROCEDURE sdispatch, ddispatch, xdispatch
        END INTERFACE

        ! Overloaded BLAS routines (single/double precision).
        INTERFACE libxsmm_blasmm
          MODULE PROCEDURE libxsmm_sblasmm, libxsmm_dblasmm
        END INTERFACE

        ! Overloaded auto-dispatch routines (single/double precision).
        INTERFACE libxsmm_mm
          MODULE PROCEDURE libxsmm_smm, libxsmm_dmm
        END INTERFACE

        ! Type of a function generated for a specific M, N, K, and Alpha, Beta.
        ABSTRACT INTERFACE
          PURE SUBROUTINE LIBXSMM_SMM_FUNCTION(a, b, c, xargs) BIND(C)
            IMPORT :: C_FLOAT, LIBXSMM_SGEMM_XARGS
            REAL(C_FLOAT), INTENT(IN) :: a(*), b(*)
            REAL(C_FLOAT), INTENT(INOUT) :: c(*)
            TYPE(LIBXSMM_SGEMM_XARGS), INTENT(IN) :: xargs
          END SUBROUTINE
          PURE SUBROUTINE LIBXSMM_DMM_FUNCTION(a, b, c, xargs) BIND(C)
            IMPORT :: C_DOUBLE, LIBXSMM_DGEMM_XARGS
            REAL(C_DOUBLE), INTENT(IN) :: a(*), b(*)
            REAL(C_DOUBLE), INTENT(INOUT) :: c(*)
            TYPE(LIBXSMM_DGEMM_XARGS), INTENT(IN) :: xargs
          END SUBROUTINE
        END INTERFACE

        !DIR$ ATTRIBUTES OFFLOAD:MIC :: sgemm, dgemm
        !DIR$ ATTRIBUTES OFFLOAD:MIC :: libxsmm_init, libxsmm_sdispatch, libxsmm_ddispatch
        !DIR$ ATTRIBUTES OFFLOAD:MIC :: libxsmm_timer_tick, libxsmm_timer_duration
        INTERFACE
          SUBROUTINE sgemm(                                             &
     &    transa, transb, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc)
            IMPORT LIBXSMM_INTEGER_TYPE, LIBXSMM_SINGLE_PRECISION
            CHARACTER(1), INTENT(IN) :: transa, transb
            INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: lda, ldb, ldc
            REAL(LIBXSMM_SINGLE_PRECISION), INTENT(IN) :: alpha, beta
            REAL(LIBXSMM_SINGLE_PRECISION), INTENT(IN) :: a(lda,*)
            REAL(LIBXSMM_SINGLE_PRECISION), INTENT(IN) :: b(ldb,*)
            REAL(LIBXSMM_SINGLE_PRECISION), INTENT(INOUT) :: c(ldc,*)
          END SUBROUTINE
          SUBROUTINE dgemm(                                             &
     &    transa, transb, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc)
            IMPORT LIBXSMM_INTEGER_TYPE, LIBXSMM_DOUBLE_PRECISION
            CHARACTER(1), INTENT(IN) :: transa, transb
            INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: m, n, k
            INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: lda, ldb, ldc
            REAL(LIBXSMM_DOUBLE_PRECISION), INTENT(IN) :: alpha, beta
            REAL(LIBXSMM_DOUBLE_PRECISION), INTENT(IN) :: a(lda,*)
            REAL(LIBXSMM_DOUBLE_PRECISION), INTENT(IN) :: b(ldb,*)
            REAL(LIBXSMM_DOUBLE_PRECISION), INTENT(INOUT) :: c(ldc,*)
          END SUBROUTINE

          ! Initialize the library; pay for setup cost at a specific point.
          SUBROUTINE libxsmm_init() BIND(C)
          END SUBROUTINE

          ! Query or JIT-generate a function; return zero if it does not exist,
          ! or if JIT is not supported (single-precision).
          TYPE(C_FUNPTR) PURE FUNCTION libxsmm_sdispatch(               &
     &    m, n, k, alpha, beta, lda, ldb, ldc, flags, prefetch) BIND(C)
            IMPORT :: C_FUNPTR, C_FLOAT, C_INT
            INTEGER(C_INT), INTENT(IN), VALUE :: m, n, k
            REAL(C_FLOAT),  INTENT(IN), VALUE :: alpha, beta
            INTEGER(C_INT), INTENT(IN), VALUE :: lda, ldb, ldc
            INTEGER(C_INT), INTENT(IN), VALUE :: flags, prefetch
          END FUNCTION
          ! Query or JIT-generate a function; return zero if it does not exist,
          ! or if JIT is not supported (double-precision).
          TYPE(C_FUNPTR) PURE FUNCTION libxsmm_ddispatch(               &
     &    m, n, k, alpha, beta, lda, ldb, ldc, flags, prefetch) BIND(C)
            IMPORT :: C_FUNPTR, C_DOUBLE, C_INT
            INTEGER(C_INT), INTENT(IN), VALUE :: m, n, k
            REAL(C_DOUBLE), INTENT(IN), VALUE :: alpha, beta
            INTEGER(C_INT), INTENT(IN), VALUE :: lda, ldb, ldc
            INTEGER(C_INT), INTENT(IN), VALUE :: flags, prefetch
          END FUNCTION

          ! Non-pure function returning the current clock tick
          ! using a platform-specific resolution.
          INTEGER(C_LONG_LONG) FUNCTION libxsmm_timer_tick() BIND(C)
            IMPORT :: C_LONG_LONG
          END FUNCTION
          ! Non-pure function (timer freq. may vary) returning
          ! the duration between two ticks (seconds).
          REAL(C_DOUBLE) FUNCTION libxsmm_timer_duration(               &
     &    tick0, tick1) BIND(C)
            IMPORT :: C_LONG_LONG, C_DOUBLE
            INTEGER(C_LONG_LONG), INTENT(IN), VALUE :: tick0, tick1
          END FUNCTION
        END INTERFACE$MNK_INTERFACE_LIST

      CONTAINS
        !DIR$ ATTRIBUTES OFFLOAD:MIC :: LIBXSMM_SGEMM_XARGS_CTOR
        TYPE(LIBXSMM_DGEMM_XARGS)                                       &
     &  PURE FUNCTION LIBXSMM_SGEMM_XARGS_CTOR(alpha, beta, pa, pb, pc)
          INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                   &
     &      T = LIBXSMM_SINGLE_PRECISION
          REAL(T), INTENT(IN), OPTIONAL :: alpha, beta
          TYPE(C_PTR), INTENT(IN), OPTIONAL :: pa, pb, pc
          LIBXSMM_SGEMM_XARGS_CTOR%alpha =                              &
     &      MERGE(REAL(LIBXSMM_ALPHA, T), alpha, .NOT.PRESENT(alpha))
          LIBXSMM_SGEMM_XARGS_CTOR%beta =                               &
     &      MERGE(REAL(LIBXSMM_ALPHA, T), beta, .NOT.PRESENT(beta))
          LIBXSMM_SGEMM_XARGS_CTOR%pa =                                 &
     &      MERGE(C_NULL_PTR, pa, .NOT.PRESENT(pa))
          LIBXSMM_SGEMM_XARGS_CTOR%pb =                                 &
     &      MERGE(C_NULL_PTR, pb, .NOT.PRESENT(pb))
          LIBXSMM_SGEMM_XARGS_CTOR%pc =                                 &
     &      MERGE(C_NULL_PTR, pc, .NOT.PRESENT(pc))
        END FUNCTION

        !DIR$ ATTRIBUTES OFFLOAD:MIC :: LIBXSMM_DGEMM_XARGS_CTOR
        TYPE(LIBXSMM_DGEMM_XARGS)                                       &
     &  PURE FUNCTION LIBXSMM_DGEMM_XARGS_CTOR(alpha, beta, pa, pb, pc)
          INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                   &
     &      T = LIBXSMM_DOUBLE_PRECISION
          REAL(T), INTENT(IN), OPTIONAL :: alpha, beta
          TYPE(C_PTR), INTENT(IN), OPTIONAL :: pa, pb, pc
          LIBXSMM_DGEMM_XARGS_CTOR%alpha =                              &
     &      MERGE(REAL(LIBXSMM_ALPHA, T), alpha, .NOT.PRESENT(alpha))
          LIBXSMM_DGEMM_XARGS_CTOR%beta =                               &
     &      MERGE(REAL(LIBXSMM_ALPHA, T), beta, .NOT.PRESENT(beta))
          LIBXSMM_DGEMM_XARGS_CTOR%pa =                                 &
     &      MERGE(C_NULL_PTR, pa, .NOT.PRESENT(pa))
          LIBXSMM_DGEMM_XARGS_CTOR%pb =                                 &
     &      MERGE(C_NULL_PTR, pb, .NOT.PRESENT(pb))
          LIBXSMM_DGEMM_XARGS_CTOR%pc =                                 &
     &      MERGE(C_NULL_PTR, pc, .NOT.PRESENT(pc))
        END FUNCTION

        !DIR$ ATTRIBUTES OFFLOAD:MIC :: libxsmm_up
        INTEGER(LIBXSMM_INTEGER_TYPE) PURE FUNCTION libxsmm_up(n, up)
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: n, up
          libxsmm_up = ((n + up - 1) / up) * up
        END FUNCTION

        !DIR$ ATTRIBUTES OFFLOAD:MIC :: libxsmm_align_value
        INTEGER(LIBXSMM_INTEGER_TYPE) PURE FUNCTION libxsmm_align_value(&
     &    n, typesize, alignment)
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: n
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: typesize
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: alignment
          libxsmm_align_value = libxsmm_up(n * typesize, alignment) /   &
     &      typesize
        END FUNCTION

        !DIR$ ATTRIBUTES OFFLOAD:MIC :: libxsmm_ld
        INTEGER(LIBXSMM_INTEGER_TYPE) PURE FUNCTION libxsmm_ld(m, n)
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: m, n
          libxsmm_ld = MERGE(m, n, 0.NE.LIBXSMM_COL_MAJOR)
        END FUNCTION

        ! Non-dispatched matrix-matrix multiplication using BLAS (single-precision).
        !DIR$ ATTRIBUTES OFFLOAD:MIC :: libxsmm_sblasmm
        SUBROUTINE libxsmm_sblasmm(m, n, k, a, b, c, xargs)
          INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                   &
     &      T = LIBXSMM_SINGLE_PRECISION
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: m, n, k
          REAL(T), INTENT(IN) :: a(:,:), b(:,:)
          REAL(T), INTENT(INOUT) :: c(:,:)
          TYPE(LIBXSMM_SGEMM_XARGS), INTENT(IN), OPTIONAL :: xargs
          IF (0.NE.LIBXSMM_COL_MAJOR) THEN
            CALL sgemm('N', 'N', m, n, k,                               &
     &        MERGE(REAL(LIBXSMM_ALPHA, T), xargs%alpha,                &
     &            .NOT.PRESENT(xargs)),                                 &
     &        a, MAX(SIZE(a, 1), m), b, MAX(SIZE(b, 1), k),             &
     &        MERGE(REAL(LIBXSMM_BETA, T), xargs%beta,                  &
     &            .NOT.PRESENT(xargs)), c,                              &
     &        MAX(SIZE(c, 1), m))
          ELSE
            CALL sgemm('N', 'N', n, m, k,                               &
     &        MERGE(REAL(LIBXSMM_ALPHA, T), xargs%alpha,                &
     &            .NOT.PRESENT(xargs)),                                 &
     &        b, MAX(SIZE(b, 2), n), a, MAX(SIZE(a, 2), k),             &
     &        MERGE(REAL(LIBXSMM_BETA, T), xargs%beta,                  &
     &            .NOT.PRESENT(xargs)), c,                              &
     &        MAX(SIZE(c, 1), n))
          ENDIF
        END SUBROUTINE

        ! Non-dispatched matrix-matrix multiplication using BLAS (double-precision).
        !DIR$ ATTRIBUTES OFFLOAD:MIC :: libxsmm_dblasmm
        SUBROUTINE libxsmm_dblasmm(m, n, k, a, b, c, xargs)
          INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                   &
     &      T = LIBXSMM_DOUBLE_PRECISION
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: m, n, k
          REAL(T), INTENT(IN) :: a(:,:), b(:,:)
          REAL(T), INTENT(INOUT) :: c(:,:)
          TYPE(LIBXSMM_DGEMM_XARGS), INTENT(IN), OPTIONAL :: xargs
          IF (0.NE.LIBXSMM_COL_MAJOR) THEN
            CALL dgemm('N', 'N', m, n, k,                               &
     &        MERGE(REAL(LIBXSMM_ALPHA, T), xargs%alpha,                &
     &            .NOT.PRESENT(xargs)),                                 &
     &        a, MAX(SIZE(a, 1), m), b, MAX(SIZE(b, 1), k),             &
     &        MERGE(REAL(LIBXSMM_BETA, T), xargs%beta,                  &
     &            .NOT.PRESENT(xargs)), c,                              &
     &        MAX(SIZE(c, 1), m))
          ELSE
            CALL dgemm('N', 'N', n, m, k,                               &
     &        MERGE(REAL(LIBXSMM_ALPHA, T), xargs%alpha,                &
     &            .NOT.PRESENT(xargs)),                                 &
     &        b, MAX(SIZE(b, 2), n), a, MAX(SIZE(a, 2), k),             &
     &        MERGE(REAL(LIBXSMM_BETA, T), xargs%beta,                  &
     &            .NOT.PRESENT(xargs)), c,                              &
     &        MAX(SIZE(c, 1), n))
          ENDIF
        END SUBROUTINE

        ! Dispatched matrix-matrix multiplication (single-precision).
        !DIR$ ATTRIBUTES OFFLOAD:MIC :: libxsmm_smm
        SUBROUTINE libxsmm_smm(m, n, k, a, b, c, xargs)
          INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                   &
     &      T = LIBXSMM_SINGLE_PRECISION
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: m, n, k
          REAL(T), INTENT(IN) :: a(:,:), b(:,:)
          REAL(T), INTENT(INOUT) :: c(:,:)
          TYPE(LIBXSMM_SGEMM_XARGS), INTENT(IN), OPTIONAL :: xargs
          !DIR$ ATTRIBUTES OFFLOAD:MIC :: xmm
          PROCEDURE(LIBXSMM_SMM_FUNCTION), POINTER :: xmm
          INTEGER(LIBXSMM_INTEGER_TYPE) :: mn
          TYPE(C_FUNPTR) :: fn
          IF (LIBXSMM_MAX_MNK.GE.(m * n * k)) THEN
            mn = libxsmm_ld(m, n)
            fn = libxsmm_sdispatch(m, n, k,                             &
     &        MERGE(REAL(LIBXSMM_ALPHA, T), xargs%alpha,                &
     &            .NOT.PRESENT(xargs)),                                 &
     &        MERGE(REAL(LIBXSMM_BETA, T), xargs%beta,                  &
     &            .NOT.PRESENT(xargs)),                                 &
     &        mn, k, libxsmm_align_value(mn, T, LIBXSMM_ALIGNED_STORES),&
     &        LIBXSMM_GEMM_FLAG_DEFAULT, LIBXSMM_PREFETCH)
            IF (C_ASSOCIATED(fn)) THEN
              CALL C_F_PROCPOINTER(fn, xmm)
              CALL xmm(a, b, c, xargs)
            ELSE
              CALL libxsmm_sblasmm(m, n, k, a, b, c, xargs)
            ENDIF
          ELSE
            CALL libxsmm_sblasmm(m, n, k, a, b, c, xargs)
          ENDIF
        END SUBROUTINE

        ! Dispatched matrix-matrix multiplication (double-precision).
        !DIR$ ATTRIBUTES OFFLOAD:MIC :: libxsmm_dmm
        SUBROUTINE libxsmm_dmm(m, n, k, a, b, c, xargs)
          INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                   &
     &      T = LIBXSMM_DOUBLE_PRECISION
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: m, n, k
          REAL(T), INTENT(IN) :: a(:,:), b(:,:)
          REAL(T), INTENT(INOUT) :: c(:,:)
          TYPE(LIBXSMM_DGEMM_XARGS), INTENT(IN), OPTIONAL :: xargs
          !DIR$ ATTRIBUTES OFFLOAD:MIC :: xmm
          PROCEDURE(LIBXSMM_DMM_FUNCTION), POINTER :: xmm
          INTEGER(LIBXSMM_INTEGER_TYPE) :: mn
          TYPE(C_FUNPTR) :: fn
          IF (LIBXSMM_MAX_MNK.GE.(m * n * k)) THEN
            mn = libxsmm_ld(m, n)
            fn = libxsmm_ddispatch(m, n, k,                             &
     &        MERGE(REAL(LIBXSMM_ALPHA, T), xargs%alpha,                &
     &            .NOT.PRESENT(xargs)),                                 &
     &        MERGE(REAL(LIBXSMM_BETA, T), xargs%beta,                  &
     &            .NOT.PRESENT(xargs)),                                 &
     &        mn, k, libxsmm_align_value(mn, T, LIBXSMM_ALIGNED_STORES),&
     &        LIBXSMM_GEMM_FLAG_DEFAULT, LIBXSMM_PREFETCH)
            IF (C_ASSOCIATED(fn)) THEN
              CALL C_F_PROCPOINTER(fn, xmm)
              CALL xmm(a, b, c, xargs)
            ELSE
              CALL libxsmm_dblasmm(m, n, k, a, b, c, xargs)
            ENDIF
          ELSE
            CALL libxsmm_dblasmm(m, n, k, a, b, c, xargs)
          ENDIF
        END SUBROUTINE

        !DIR$ ATTRIBUTES OFFLOAD:MIC :: sdispatch
        TYPE(C_FUNPTR) PURE FUNCTION sdispatch(                         &
     &  m, n, k, alpha, beta, lda, ldb, ldc, flags, prefetch)
          INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                   &
     &      T = LIBXSMM_SINGLE_PRECISION
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN), OPTIONAL ::        &
     &      lda, ldb, ldc, flags, prefetch
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: m, n, k
          REAL(T), INTENT(IN), OPTIONAL :: beta
          REAL(T), INTENT(IN) :: alpha
          sdispatch = libxsmm_sdispatch(m, n, k, alpha,                 &
     &      MERGE(REAL(LIBXSMM_BETA, T), beta,                          &
     &          .NOT.PRESENT(beta)),                                    &
     &      MERGE(INT(0, LIBXSMM_INTEGER_TYPE), lda,                    &
     &          .NOT.PRESENT(lda)),                                     &
     &      MERGE(INT(0, LIBXSMM_INTEGER_TYPE), ldb,                    &
     &          .NOT.PRESENT(ldb)),                                     &
     &      MERGE(INT(0, LIBXSMM_INTEGER_TYPE), ldc,                    &
     &          .NOT.PRESENT(ldc)),                                     &
     &      MERGE(LIBXSMM_GEMM_FLAG_DEFAULT, flags,                     &
     &          .NOT.PRESENT(flags)),                                   &
     &      MERGE(LIBXSMM_PREFETCH, prefetch,                           &
     &          .NOT.PRESENT(prefetch)))
        END FUNCTION

        !DIR$ ATTRIBUTES OFFLOAD:MIC :: ddispatch
        TYPE(C_FUNPTR) PURE FUNCTION ddispatch(                         &
     &  m, n, k, alpha, beta, lda, ldb, ldc, flags, prefetch)
          INTEGER(LIBXSMM_INTEGER_TYPE), PARAMETER ::                   &
     &      T = LIBXSMM_DOUBLE_PRECISION
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN), OPTIONAL ::        &
     &      lda, ldb, ldc, flags, prefetch
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: m, n, k
          REAL(T), INTENT(IN), OPTIONAL :: beta
          REAL(T), INTENT(IN) :: alpha
          ddispatch = libxsmm_ddispatch(m, n, k, alpha,                 &
     &      MERGE(REAL(LIBXSMM_BETA, T), beta,                          &
     &          .NOT.PRESENT(beta)),                                    &
     &      MERGE(INT(0, LIBXSMM_INTEGER_TYPE), lda,                    &
     &          .NOT.PRESENT(lda)),                                     &
     &      MERGE(INT(0, LIBXSMM_INTEGER_TYPE), ldb,                    &
     &          .NOT.PRESENT(ldb)),                                     &
     &      MERGE(INT(0, LIBXSMM_INTEGER_TYPE), ldc,                    &
     &          .NOT.PRESENT(ldc)),                                     &
     &      MERGE(LIBXSMM_GEMM_FLAG_DEFAULT, flags,                     &
     &          .NOT.PRESENT(flags)),                                   &
     &      MERGE(LIBXSMM_PREFETCH, prefetch,                           &
     &          .NOT.PRESENT(prefetch)))
        END FUNCTION

        TYPE(C_FUNPTR) PURE FUNCTION xdispatch(m, n, k, type)
          INTEGER(LIBXSMM_INTEGER_TYPE), INTENT(IN) :: m, n, k, type
          xdispatch = MERGE(                                            &
     &      libxsmm_dispatch(m, n, k,                                   &
     &          REAL(LIBXSMM_ALPHA, LIBXSMM_DOUBLE_PRECISION),          &
     &          REAL(LIBXSMM_BETA, LIBXSMM_DOUBLE_PRECISION)),          &
     &      libxsmm_dispatch(m, n, k,                                   &
     &          REAL(LIBXSMM_ALPHA, LIBXSMM_SINGLE_PRECISION),          &
     &          REAL(LIBXSMM_BETA, LIBXSMM_SINGLE_PRECISION)),          &
     &      LIBXSMM_DOUBLE_PRECISION.EQ.type)
        END FUNCTION
      END MODULE
