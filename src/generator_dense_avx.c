/******************************************************************************
** Copyright (c) 2014-2015, Intel Corporation                                **
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
/* Alexander Heinecke (Intel Corp.)
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "generator_common.h"
#include "generator_dense_common.h"
#include "generator_dense_instructions.h"

void libxsmm_generator_dense_avx_load_C_MxN(char**                          io_generated_code,
                                            const libxsmm_gp_reg_mapping*   i_gp_reg_mapping,
                                            const libxsmm_xgemm_descriptor* i_xgemm_desc,
                                            const char*                     i_vload_instr,
                                            const char*                     i_vxor_instr,
                                            const char*                     i_vector_name,
                                            const unsigned int              i_m_blocking,
                                            const unsigned int              i_n_blocking,
                                            const unsigned int              i_vector_length,
                                            const unsigned int              i_datatype_size) {
#ifndef NDEGUG
  /* Do some test if it's possible to generated the requested code. 
     This is not done in release mode and therefore bad
     things might happen.... HUAAH */ 
  if ( (i_n_blocking * i_m_blocking > 12) || (i_n_blocking < 1) || (i_m_blocking < 1) ) {
    fprintf(stderr, "LIBXSMM ERROR, libxsmm_generator_dense_avx_load_MxN, register blocking is invalid!!!\n");
    exit(-1);
  }
#endif

  /* register blocking counter in n */
  unsigned int l_n = 0;
  /* register blocking counter in m */
  unsigned int l_m = 0;
  /* start register of accumulator */
  unsigned int l_vec_reg_acc_start = 16 - (i_n_blocking * i_m_blocking);

  /* load C accumulator */
  if (i_xgemm_desc->beta == 1) {
    /* adding to C, so let's load C */
    for ( l_n = 0; l_n < i_n_blocking; l_n++ ) {
      for ( l_m = 0; l_m < i_m_blocking; l_m++ ) {
        libxsmm_instruction_vec_move( io_generated_code, 
                                      i_vload_instr, i_gp_reg_mapping->gp_reg_c, ((l_n * i_xgemm_desc->ldc) + (l_m * i_vector_length)) * i_datatype_size, i_vector_name, l_vec_reg_acc_start + l_m + (i_m_blocking * l_n), 0 );
      }
    }
  } else {
    /* overwriting C, so let's xout accumulator */
    for ( l_n = 0; l_n < i_n_blocking; l_n++ ) {
      for ( l_m = 0; l_m < i_m_blocking; l_m++ ) { 
        libxsmm_instruction_vec_compute_reg( io_generated_code, 
                                             i_vxor_instr, i_vector_name, l_vec_reg_acc_start + l_m + (i_m_blocking * l_n), l_vec_reg_acc_start + l_m + (i_m_blocking * l_n), l_vec_reg_acc_start + l_m + (i_m_blocking * l_n) );
      }
    }
  }
}

void libxsmm_generator_dense_avx_store_C_MxN(char**                          io_generated_code,
                                             const libxsmm_gp_reg_mapping*   i_gp_reg_mapping,
                                             const libxsmm_xgemm_descriptor* i_xgemm_desc,
                                             const char*                     i_vstore_instr,
                                             const char*                     i_prefetch_instr,
                                             const char*                     i_vector_name,
                                             const unsigned int              i_m_blocking,
                                             const unsigned int              i_n_blocking,
                                             const unsigned int              i_vector_length,
                                             const unsigned int              i_datatype_size) {
  /* @TODO fix this test */ 
#ifndef NDEBUG
  if ( (i_n_blocking > 3) || (i_n_blocking < 1) ) {
    fprintf(stderr, "LIBXSMM ERROR, libxsmm_generator_dense_avx_store_MxN, i_n_blocking smaller 1 or larger 3!!!\n");
    exit(-1);
  }
#endif

  /* register blocking counter in n */
  unsigned int l_n = 0;
  /* register blocking counter in m */
  unsigned int l_m = 0;
  /* start register of accumulator */
  unsigned int l_vec_reg_acc_start = 16 - (i_n_blocking * i_m_blocking);

  /* storing C accumulator */
  /* adding to C, so let's load C */
  for ( l_n = 0; l_n < i_n_blocking; l_n++ ) {
    for ( l_m = 0; l_m < i_m_blocking; l_m++ ) {
      libxsmm_instruction_vec_move( io_generated_code, 
                                    i_vstore_instr, i_gp_reg_mapping->gp_reg_c, ((l_n * i_xgemm_desc->ldc) + (l_m * i_vector_length)) * i_datatype_size, i_vector_name, l_vec_reg_acc_start + l_m + (i_m_blocking * l_n), 1 );
    }
  }

  if ( strcmp( i_xgemm_desc->prefetch, "BL2viaC" ) == 0 ) {
    /* determining how many prefetches we need M direction as we just need one prefetch per cache line */
    unsigned int l_m_advance = 64/(i_vector_length * i_datatype_size); /* 64: hardcoded cache line length */
        
    for ( l_n = 0; l_n < i_n_blocking; l_n++) {
      for (l_m = 0; l_m < i_m_blocking; l_m += l_m_advance ) {
        libxsmm_instruction_prefetch( io_generated_code, 
                                      i_prefetch_instr, i_gp_reg_mapping->gp_reg_b_prefetch, ((l_n * i_xgemm_desc->ldc) + (l_m * i_vector_length)) * i_datatype_size);
      }
    }
  }
}

void libxsmm_generator_dense_avx_compute_MxN(char**                          io_generated_code,
                                             const libxsmm_gp_reg_mapping*   i_gp_reg_mapping,
                                             const libxsmm_xgemm_descriptor* i_xgemm_desc,
                                             const char*                     i_a_load_instr,
                                             const char*                     i_b_load_instr,
                                             const char*                     i_add_instr,
                                             const char*                     i_vmul_instr,
                                             const char*                     i_vadd_instr,                                          
                                             const char*                     i_vector_name,
                                             const unsigned int              i_m_blocking,
                                             const unsigned int              i_n_blocking,
                                             const unsigned int              i_vector_length,
                                             const unsigned int              i_datatype_size,
                                             const int                       i_offset) {
  /* @TODO fix this test */
#ifndef NDEBUG
  if ( (i_n_blocking > 3) || (i_n_blocking < 1) ) {
    fprintf(stderr, "LIBXSMM ERROR, libxsmm_generator_dense_avx_compute_MxN, i_n_blocking smaller 1 or larger 3!!!\n");
    exit(-1);
  }
#endif

  /* register blocking counter in n */
  unsigned int l_n = 0;
  /* register blocking counter in m */
  unsigned int l_m = 0;
  /* start register of accumulator */
  unsigned int l_vec_reg_acc_start = 16 - (i_n_blocking * i_m_blocking);

  /* broadcast from B -> into vec registers 0 to i_n_blocking */
  if ( i_offset != (-1) ) { 
    for ( l_n = 0; l_n < i_n_blocking; l_n++ ) {
      libxsmm_instruction_vec_move( io_generated_code, 
                                    i_b_load_instr, i_gp_reg_mapping->gp_reg_b, (i_datatype_size * i_offset) + (i_xgemm_desc->ldb * l_n * i_datatype_size), i_vector_name, l_n, 0 );
    }
  } else {
    for ( l_n = 0; l_n < i_n_blocking; l_n++ ) {
      libxsmm_instruction_vec_move( io_generated_code, 
                                    i_b_load_instr, i_gp_reg_mapping->gp_reg_b, i_xgemm_desc->ldb * l_n * i_datatype_size, i_vector_name, l_n, 0 );
    }
    libxsmm_instruction_alu_imm( io_generated_code,
                                 i_add_instr, i_gp_reg_mapping->gp_reg_b, i_datatype_size );
  }

  /* load column vectors of A and multiply with all broadcasted row entries of B */
  for ( l_m = 0; l_m < i_m_blocking ; l_m++ ) {
    libxsmm_instruction_vec_move( io_generated_code, 
                                  i_a_load_instr, i_gp_reg_mapping->gp_reg_a, i_datatype_size * i_vector_length * l_m, i_vector_name, i_n_blocking, 0 );
    for ( l_n = 0; l_n < i_n_blocking; l_n++ ) {
      /* post increment early */
      if ( (l_m == (i_m_blocking-1)) && (l_n == 0) ) {
        libxsmm_instruction_alu_imm( io_generated_code,
                                     i_add_instr, i_gp_reg_mapping->gp_reg_a, i_xgemm_desc->lda*i_datatype_size );
      }
      /* issue fma / mul-add */
      /* @TODO add support for mul/add */
      libxsmm_instruction_vec_compute_reg( io_generated_code, 
                                           i_vmul_instr, i_vector_name, i_n_blocking, l_n, l_vec_reg_acc_start + l_m + (i_m_blocking * l_n) );
    }
  }
}

void libxsmm_generator_dense_avx_kernel(char**                          io_generated_code,
                                        const libxsmm_xgemm_descriptor* i_xgemm_desc,
                                        const char*                     i_arch,
                                        const unsigned int              i_vector_length,
                                        const char*                     i_vector_name) {
  /* define gp register mapping */
  libxsmm_gp_reg_mapping l_gp_reg_mapping;
  libxsmm_reset_x86_gp_reg_mapping( &l_gp_reg_mapping );
  l_gp_reg_mapping.gp_reg_a = LIBXSMM_X86_GP_REG_R8;
  l_gp_reg_mapping.gp_reg_b = LIBXSMM_X86_GP_REG_R9;
  l_gp_reg_mapping.gp_reg_c = LIBXSMM_X86_GP_REG_R10;
  l_gp_reg_mapping.gp_reg_a_prefetch = LIBXSMM_X86_GP_REG_R11;
  l_gp_reg_mapping.gp_reg_b_prefetch = LIBXSMM_X86_GP_REG_R15;
  l_gp_reg_mapping.gp_reg_mloop = LIBXSMM_X86_GP_REG_R12;
  l_gp_reg_mapping.gp_reg_nloop = LIBXSMM_X86_GP_REG_R13;
  l_gp_reg_mapping.gp_reg_kloop = LIBXSMM_X86_GP_REG_R14; 
  
  unsigned int l_n_done = 0;
  unsigned int l_n_done_old = 0;
  unsigned int l_n_blocking = 3;

  char* l_a_vmove_instr = "vmovapd";
  char* l_b_vmove_instr = "vbroadcastsd";
  char* l_c_vmove_instr = "vmovapd";
  char* l_prefetch_instr = "prefetch1";
  char* l_vxor_instr = "vxorpd";
  char* l_vmul_instr = "vfmadd231pd";
  char* l_vadd_instr = NULL;
  char* l_alu_add_instr = "addq";

  unsigned int l_datatype_size = 8;
  
  /* open asm */
  libxsmm_generator_dense_sse_avx_open_instruction_stream( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc->prefetch );
  
  /* apply n_blocking */
  while (l_n_done != i_xgemm_desc->n) {
    l_n_done_old = l_n_done;
    l_n_done = l_n_done + (((i_xgemm_desc->n - l_n_done_old) / l_n_blocking) * l_n_blocking);

    if (l_n_done != l_n_done_old && l_n_done > 0) {

      libxsmm_generator_dense_header_nloop( io_generated_code, &l_gp_reg_mapping, l_n_blocking );
  
      unsigned int l_k_blocking = 4;
      unsigned int l_k_threshold = 30;
      unsigned int l_m_done = 0;
      unsigned int l_m_done_old = 0;
      unsigned int l_m_blocking = 16;

      /* apply m_blocking */
      while (l_m_done != i_xgemm_desc->m) {
#if 0
        if (mDone == 0) { 
          mDone_old = mDone;
          if (M == 56) {
            mDone = 32;
          } else {
            mDone = mDone + (((M - mDone_old) / m_blocking) * m_blocking);
          }  
        } else {
#endif
        /* @TODO enable upper part again later */
        l_m_done_old = l_m_done;
        l_m_done = l_m_done + (((i_xgemm_desc->m - l_m_done_old) / l_m_blocking) * l_m_blocking);
  
        if ( (l_m_done != l_m_done_old) && (l_m_done > 0) ) {
          libxsmm_generator_dense_header_mloop( io_generated_code, &l_gp_reg_mapping, l_m_blocking );
          libxsmm_generator_dense_avx_load_C_MxN( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc, l_c_vmove_instr, l_vxor_instr, i_vector_name,
                                                  l_m_blocking/i_vector_length, l_n_blocking, i_vector_length, l_datatype_size);

          /* apply multiple k_blocking strategies */
          /* 1. we are larger the k_threshold and a multple of a predefined blocking parameter */
          if ((i_xgemm_desc->k % l_k_blocking) == 0 && i_xgemm_desc->k > l_k_threshold) {
            libxsmm_generator_dense_header_kloop( io_generated_code, &l_gp_reg_mapping, l_m_blocking, l_k_blocking);
            
            unsigned int l_k;
            for ( l_k = 0; l_k < l_k_blocking; l_k++) {
	      libxsmm_generator_dense_avx_compute_MxN(io_generated_code, &l_gp_reg_mapping, i_xgemm_desc, l_a_vmove_instr, l_b_vmove_instr, l_alu_add_instr, 
                                                      l_vmul_instr, l_vadd_instr, i_vector_name, l_m_blocking/i_vector_length, l_n_blocking,
                                                      i_vector_length, l_datatype_size, -1);
            }

            libxsmm_generator_dense_footer_kloop( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc, l_m_blocking, l_datatype_size, 1 );
          } else {
            /* 2. we want to fully unroll below the threshold */
            if (i_xgemm_desc->k <= l_k_threshold) {
              unsigned int l_k;
              for ( l_k = 0; l_k < i_xgemm_desc->k; l_k++) {
	        libxsmm_generator_dense_avx_compute_MxN(io_generated_code, &l_gp_reg_mapping, i_xgemm_desc, l_a_vmove_instr, l_b_vmove_instr, l_alu_add_instr, 
                                                        l_vmul_instr, l_vadd_instr, i_vector_name, l_m_blocking/i_vector_length, l_n_blocking,
                                                        i_vector_length, l_datatype_size, l_k);
	      }
            /* 3. we are large than the threshold but not a multiple of the blocking factor -> largest possible blocking + remainder handling */
            } else {
	      unsigned int l_max_blocked_k = ((i_xgemm_desc->k)/l_k_blocking)*l_k_blocking;
	      if ( l_max_blocked_k > 0 ) {
	        libxsmm_generator_dense_header_kloop( io_generated_code, &l_gp_reg_mapping, l_m_blocking, l_k_blocking);
               
                unsigned int l_k;
                for ( l_k = 0; l_k < l_k_blocking; l_k++) {
	          libxsmm_generator_dense_avx_compute_MxN(io_generated_code, &l_gp_reg_mapping, i_xgemm_desc, l_a_vmove_instr, l_b_vmove_instr, l_alu_add_instr, 
                                                          l_vmul_instr, l_vadd_instr, i_vector_name, l_m_blocking/i_vector_length, l_n_blocking,
                                                          i_vector_length, l_datatype_size, -1);
                }

	        libxsmm_generator_dense_footer_kloop( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc, l_m_blocking, l_datatype_size, 0 );
	      }
	      if (l_max_blocked_k > 0 ) {
                libxsmm_instruction_alu_imm( io_generated_code, "subq", l_gp_reg_mapping.gp_reg_b, l_max_blocked_k * l_datatype_size );
	      }
              unsigned int l_k;
	      for ( l_k = l_max_blocked_k; l_k < i_xgemm_desc->k; l_k++) {
	        libxsmm_generator_dense_avx_compute_MxN(io_generated_code, &l_gp_reg_mapping, i_xgemm_desc, l_a_vmove_instr, l_b_vmove_instr, l_alu_add_instr, 
                                                        l_vmul_instr, l_vadd_instr, i_vector_name, l_m_blocking/i_vector_length, l_n_blocking,
                                                        i_vector_length, l_datatype_size, l_k);
	      }
            }
          }

          libxsmm_generator_dense_avx_store_C_MxN( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc, l_c_vmove_instr, l_prefetch_instr, i_vector_name,
                                                   l_m_blocking/i_vector_length, l_n_blocking, i_vector_length, l_datatype_size);
          libxsmm_generator_dense_footer_mloop( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc, l_m_blocking, l_datatype_size);
        }

        /* switch to next smaller m_blocking */
        if (l_m_blocking == 2) {
          l_m_blocking = 1;
        } else if (l_m_blocking == 4) {
          l_m_blocking = 2;
        } else if (l_m_blocking == 8) {
          l_m_blocking = 4;
        } else if (l_m_blocking == 12) {
          l_m_blocking = 8;
        } else if (l_m_blocking == 16) {
          l_m_blocking = 12;
        } else {
          /* we are done with m_blocking */
        }
      }

      libxsmm_generator_dense_footer_nloop( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc, l_n_blocking, l_datatype_size);
    }

    /* switch to a different, smaller n_blocking */
    if (l_n_blocking == 2) {
      l_n_blocking = 1;
    } else if (l_n_blocking == 3) {
      l_n_blocking = 2;
    } else {
      /* we are done with n_blocking */ 
    }
  }

  /* close asm */
  libxsmm_generator_dense_sse_avx_close_instruction_stream( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc->prefetch );
}

void libxsmm_generator_dense_avx(char**                          io_generated_code,
                                 const libxsmm_xgemm_descriptor* i_xgemm_desc,
                                 const char*                     i_arch ) {
  unsigned int l_vector_length = 0;
  char* l_vector_name = NULL;
  libxsmm_xgemm_descriptor l_xgemm_desc_mod = *i_xgemm_desc;

  /* determining vector length depending on architecture and precision */
  if ( (strcmp(i_arch, "wsm") == 0) && (l_xgemm_desc_mod.single_precision == 0) ) {
    l_vector_length = 2;
    l_vector_name = "xmm";
  } else if ( (strcmp(i_arch, "wsm") == 0) && (l_xgemm_desc_mod.single_precision == 1) ) {
    l_vector_length = 4;
    l_vector_name = "xmm";
  } else if ( (strcmp(i_arch, "snb") == 0) && (l_xgemm_desc_mod.single_precision == 0) ) {
    l_vector_length = 4;
    l_vector_name = "ymm";
  } else if ( (strcmp(i_arch, "snb") == 0) && (l_xgemm_desc_mod.single_precision == 1) ) {
    l_vector_length = 8;
    l_vector_name = "ymm";
  } else if ( (strcmp(i_arch, "hsw") == 0) && (l_xgemm_desc_mod.single_precision == 0) ) {
    l_vector_length = 4;
    l_vector_name = "ymm";
  } else if ( (strcmp(i_arch, "hsw") == 0) && (l_xgemm_desc_mod.single_precision == 1) ) {
    l_vector_length = 8;
    l_vector_name = "ymm";
  } else {
    fprintf(stderr, "received non-valid arch and precsoin in libxsmm_generator_dense_sse_avx\n");
    exit(-1);
  }
 
  /* derive if alignment is possible */
  if ( (l_xgemm_desc_mod.lda % l_vector_length) == 0 ) {
    l_xgemm_desc_mod.aligned_a = 1;
  }
  if ( (l_xgemm_desc_mod.ldc % l_vector_length) == 0 ) {
    l_xgemm_desc_mod.aligned_c = 1;
  }

  /* enforce possible external overwrite */
  l_xgemm_desc_mod.aligned_a = l_xgemm_desc_mod.aligned_a && i_xgemm_desc->aligned_a;
  l_xgemm_desc_mod.aligned_c = l_xgemm_desc_mod.aligned_c && i_xgemm_desc->aligned_c;

  /* call actual kernel generation with revided parameters */
  libxsmm_generator_dense_avx_kernel(io_generated_code, &l_xgemm_desc_mod, i_arch, l_vector_length, l_vector_name);
}
