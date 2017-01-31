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
/* Kunal Banerjee (Intel Corp.)
******************************************************************************/

  int total_tiles = handle->cwino_bwd.itiles*handle->cwino_bwd.jtiles;
  LIBXSMM_VLA_DECL(4, float, output, outp, handle->ifwp, handle->blocksifm, TDVLEN);
  LIBXSMM_VLA_DECL(5, float, input, toutp, ALPHA, (handle->blocksifm/VRATIO)*handle->cwino_bwd.bimg, total_tiles, FDVLEN);
  float Ow[total_tiles][ALPHA][ALPHA][FDVLEN];
  float O[ALPHA - 2][ALPHA - 2][FDVLEN];
  int i;
  int j;
  int ti;
  int tj;
  int r;
  int k;
  int xdim;
  int ydim;
  float t00[FDVLEN];
  float t01[FDVLEN];
  float t10[FDVLEN];
  float t11[FDVLEN];
  float t20[FDVLEN];
  float t21[FDVLEN];
  float t30[FDVLEN];
  float t31[FDVLEN];

  for (j = 0; j < ALPHA; j++) {
    for (i = 0; i < ALPHA; i++) {
      for (tj = 0; tj < handle->cwino_bwd.jtiles; tj++) {
        for (ti = 0; ti < handle->cwino_bwd.itiles; ti++) {
          LIBXSMM_PRAGMA_SIMD
          for (k = 0; k < FDVLEN; k++) {
            Ow[tj*handle->cwino_bwd.itiles + ti][j][i][k] =
              LIBXSMM_VLA_ACCESS(5, input, j, i, 0, tj*handle->cwino_bwd.itiles + ti, k, ALPHA, (handle->blocksifm/VRATIO)*handle->cwino_bwd.bimg, total_tiles, FDVLEN);
          }
        }
      }
    }
  }
  for (tj = 0; tj < handle->cwino_bwd.jtiles; tj++) {
    for (ti = 0; ti < handle->cwino_bwd.itiles; ti++) {
      /*trans_O_2x2_3x3(ALPHA-2, FDVLEN, Ow[tj*handle->cwino_bwd.itiles + ti], O);*/

      /* inline code start */
      LIBXSMM_PRAGMA_SIMD
      for (i = 0; i < FDVLEN; i++) {
        t00[i] = Ow[tj*handle->cwino_bwd.itiles + ti][0][0][i] + Ow[tj*handle->cwino_bwd.itiles + ti][0][1][i] + Ow[tj*handle->cwino_bwd.itiles + ti][0][2][i];
        t01[i] = Ow[tj*handle->cwino_bwd.itiles + ti][0][1][i] - Ow[tj*handle->cwino_bwd.itiles + ti][0][2][i] - Ow[tj*handle->cwino_bwd.itiles + ti][0][3][i];
        t10[i] = Ow[tj*handle->cwino_bwd.itiles + ti][1][0][i] + Ow[tj*handle->cwino_bwd.itiles + ti][1][1][i] + Ow[tj*handle->cwino_bwd.itiles + ti][1][2][i];
        t11[i] = Ow[tj*handle->cwino_bwd.itiles + ti][1][1][i] - Ow[tj*handle->cwino_bwd.itiles + ti][1][2][i] - Ow[tj*handle->cwino_bwd.itiles + ti][1][3][i];
        t20[i] = Ow[tj*handle->cwino_bwd.itiles + ti][2][0][i] + Ow[tj*handle->cwino_bwd.itiles + ti][2][1][i] + Ow[tj*handle->cwino_bwd.itiles + ti][2][2][i];
        t21[i] = Ow[tj*handle->cwino_bwd.itiles + ti][2][1][i] - Ow[tj*handle->cwino_bwd.itiles + ti][2][2][i] - Ow[tj*handle->cwino_bwd.itiles + ti][2][3][i];
        t30[i] = Ow[tj*handle->cwino_bwd.itiles + ti][3][0][i] + Ow[tj*handle->cwino_bwd.itiles + ti][3][1][i] + Ow[tj*handle->cwino_bwd.itiles + ti][3][2][i];
        t31[i] = Ow[tj*handle->cwino_bwd.itiles + ti][3][1][i] - Ow[tj*handle->cwino_bwd.itiles + ti][3][2][i] - Ow[tj*handle->cwino_bwd.itiles + ti][3][3][i];
        O[0][0][i] = t00[i] + t10[i] + t20[i];
        O[0][1][i] = t01[i] + t11[i] + t21[i];
        O[1][0][i] = t10[i] - t20[i] - t30[i];
        O[1][1][i] = t11[i] - t21[i] - t31[i];
      }
      /* inline code end */

      for (j = 0; j < ALPHA-2; j++) {
        ydim = tj*(ALPHA - 2) + j;
        if (ydim >= handle->desc.H) continue;
        for (i = 0; i < ALPHA-2; i++) {
          xdim = ti*(ALPHA - 2) + i;
          if (xdim >= handle->desc.W) continue;
          for (r = 0; r < VRATIO; r++) {
            LIBXSMM_PRAGMA_SIMD
            for (k = 0; k < TDVLEN; k++) {
              LIBXSMM_VLA_ACCESS(4, output, ydim + handle->desc.pad_h, xdim + handle->desc.pad_w, r, k, handle->ifwp, handle->blocksifm, TDVLEN) +=
                O[j][i][r*TDVLEN + k];
            }
          }
        }
      }
    }
  }
