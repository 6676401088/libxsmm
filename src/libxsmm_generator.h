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

#ifndef LIBXSMM_GENERATOR_H
#define LIBXSMM_GENERATOR_H

/* struct for storing the current xgemm description
   which should be generated */
typedef struct libxsmm_xgemm_descriptor_struct {
  unsigned int m;
  unsigned int n;
  unsigned int k;
  unsigned int lda;
  unsigned int ldb;
  unsigned int ldc;
  unsigned int alpha;
  unsigned int beta;
  unsigned int trans_a;
  unsigned int trans_b;
  unsigned int aligned_a;
  unsigned int aligned_c;
  unsigned int single_precision;
  char prefetch[32]; /* TODO do this with ints as well */
} libxsmm_xgemm_descriptor;

void libxsmm_generator_dense_kernel(char**                          io_generated_code,
                                    const libxsmm_xgemm_descriptor* i_xgemm_desc,
                                    const char*                     i_arch );

void libxsmm_generator_dense(const char*                     i_file_out,
                             const char*                     i_routine_name,
                             const libxsmm_xgemm_descriptor* i_xgemm_desc,
                             const char*                     i_arch );

void libxsmm_generator_sparse(const char*                     i_file_out,
                              const char*                     i_routine_name,
                              const libxsmm_xgemm_descriptor* i_xgemm_desc,
                              const char*                     i_arch, 
                              const char*                     i_file_in );

#endif /* LIBXSMM_GENERATOR_H */

