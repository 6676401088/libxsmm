###############################################################################
## Copyright (c) 2013-2014, Intel Corporation                                ##
## All rights reserved.                                                      ##
##                                                                           ##
## Redistribution and use in source and binary forms, with or without        ##
## modification, are permitted provided that the following conditions        ##
## are met:                                                                  ##
## 1. Redistributions of source code must retain the above copyright         ##
##    notice, this list of conditions and the following disclaimer.          ##
## 2. Redistributions in binary form must reproduce the above copyright      ##
##    notice, this list of conditions and the following disclaimer in the    ##
##    documentation and/or other materials provided with the distribution.   ##
## 3. Neither the name of the copyright holder nor the names of its          ##
##    contributors may be used to endorse or promote products derived        ##
##    from this software without specific prior written permission.          ##
##                                                                           ##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       ##
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         ##
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     ##
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      ##
## HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    ##
## SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  ##
## TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    ##
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    ##
## LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      ##
## NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        ##
## SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              ##
###############################################################################
## Christopher Dahnken (Intel Corp.), Hans Pabst (Intel Corp.),
## Alfio Lazzaro (CRAY Inc.), and Gilles Fourestey (CSCS)
###############################################################################
import math
import sys


def create_symmetric_interface(dimsM, dimsN, dimsK, RowMajor):
    print "#include \"xsmm_knc.h\""
    print "#include <stdlib.h>"
    print
    print "#if defined(MKL_DIRECT_CALL_SEQ) || defined(MKL_DIRECT_CALL)"
    print "# include <mkl.h>"
    print "#else"
    print "# if defined(__cplusplus)"
    print "extern \"C\""
    print "# endif"
    print "void dgemm(const char*, const char*, const int*, const int*, const int*,"
    print "  const double*, const double*, const int*, const double*, const int*,"
    print "  const double*, double*, const int*);"
    print "#endif"
    print
    print "#ifdef __cplusplus"
    print "extern \"C\" {"
    print "#endif"
    print
    print
    print "int compareints(const void* a, const void* b)"
    print "{"
    print "  return *((const int*)a) - *((const int*)b);"
    print "}"
    print
    print
    print "dc_smm_dnn_function_type dc_smm_dnn_function(int M, int N, int K)"
    print "{"
    print "  static const int index_m[] = { " + str(dimsM).strip("[]") + " }, nm = sizeof(index_m) / sizeof(*index_m);"
    print "  static const int index_n[] = { " + str(dimsN).strip("[]") + " }, nn = sizeof(index_n) / sizeof(*index_n);"
    print "  static const int index_k[] = { " + str(dimsK).strip("[]") + " }, nk = sizeof(index_k) / sizeof(*index_k);"
    print "  static const dc_smm_dnn_function_type functions[] = {"
    for m in dimsM:
        for n in dimsN:
           sys.stdout.write("    ")
           for k in dimsK:
                sys.stdout.write("dc_smm_dnn_" + str(m) + "_" + str(n) + "_" + str(k) + ", ")
           print "// m = %d" % m
    print "  };"
    print
    print "  int m, n, k;"
    print "  return ((m = (int*)bsearch(&M, index_m, nm, sizeof(*index_m), compareints) - index_m) >= 0"
    print "       && (n = (int*)bsearch(&N, index_n, nn, sizeof(*index_n), compareints) - index_n) >= 0"
    print "       && (k = (int*)bsearch(&K, index_k, nk, sizeof(*index_k), compareints) - index_k) >= 0)"
    print "    ? functions[nk*(m*nn+n)+k]"
    print "    : 0;"
    print "}"
    print
    print
    print "void xsmm_dnn(int M, int N, int K, const double* a, const double* b, double* c)"
    print "{"
    print "  const dc_smm_dnn_function_type function = dc_smm_dnn_function(M, N, K);"
    print "  if (function) {"
    print "    (*function)(a, b, c);"
    print "  }"
    print "  else {"
    print "    double alpha = 1.0, beta = 1.0;"
    print "    char trans = 'N';"
    if 0 != RowMajor:
        print "    dgemm(&trans, &trans, &N, &M, &K, &alpha, b, &N, a, &K, &beta, c, &N);"
    else:
        print "    dgemm(&trans, &trans, &M, &N, &K, &alpha, a, &K, b, &N, &beta, c, &N);"
    print "  }"
    print "}"
    print
    print
    print "#ifdef __cplusplus"
    print "} // extern \"C\""
    print "#endif"

def load_dims(dims):
    dims = map(int, dims) ; dims.sort()
    return list(set(dims))

dimsM = load_dims(sys.argv[4:4+int(sys.argv[1])])
dimsN = load_dims(sys.argv[4+int(sys.argv[1]):4+int(sys.argv[1])+int(sys.argv[2])])
dimsK = load_dims(sys.argv[4+int(sys.argv[1])+int(sys.argv[2]):])

create_symmetric_interface(dimsM, dimsN, dimsK, int(sys.argv[3]))
