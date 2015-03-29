#! /usr/bin/env python
###############################################################################
## Copyright (c) 2013-2015, Intel Corporation                                ##
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
## Hans Pabst (Intel Corp.)
###############################################################################
from functools import reduce
import operator
import libxsmm_utilities
import sys
import os


def calc_direct_index(mnk):
    return (mnk[0] * mnk[1]) * (mnk[2] + 1) + mnk[0]


def create_dispatch_direct_function(typeflag, mnklist, maxmnk):
    print "LIBXSMM_EXTERN_C LIBXSMM_TARGET(mic) libxsmm_" + typeflag + "mm_function libxsmm_" + typeflag + "mm_dispatch(int m, int n, int k)"
    print "{"
    print "  static /*const*/ libxsmm_" + typeflag + "mm_function functions[] = {"
    sys.stdout.write("    ")
    i, m, n, last = 0, 48, 6, 0
    for mnk in mnklist:
        next = calc_direct_index(mnk)
        for j in range(last, next):
            i = i + 1
            sys.stdout.write(["0, ", "0,\n    "][0 == (i % m)])
        i = i + n
        sys.stdout.write("libxsmm_" + typeflag + "mm_" + "_".join(map(str, mnk)))
        if (mnk != mnklist[-1]): sys.stdout.write([", ", ",\n    "][n > (i % m)])
        last = next + 1
    for j in range(last, maxmnk):
        i = i + 1
        sys.stdout.write([", 0", ",\n    0"][0 == (i % m)])
    print
    print "  };"
    print "  const int i = (m * n) * (k + 1) + m;"
    print "  return functions[i];"
    print "}"


def create_dispatch_direct(mnklist, maxmnk):
    print "#include <libxsmm.h>"
    print
    print
    create_dispatch_direct_function("s", mnklist, maxmnk)
    print
    print
    create_dispatch_direct_function("d", mnklist, maxmnk)


def create_dispatch_bsearch_function(typeflag, mnklist):
    print "LIBXSMM_EXTERN_C LIBXSMM_TARGET(mic) libxsmm_" + typeflag + "mm_function libxsmm_" + typeflag + "mm_dispatch(int m, int n, int k)"
    print "{"
    print "  static /*const*/ libxsmm_" + typeflag + "mm_function functions[] = {"
    sys.stdout.write("    ")
    i, m, mnklen = 0, 6, len(mnklist)
    for mnk in mnklist:
        i = i + 1
        sys.stdout.write("libxsmm_" + typeflag + "mm_" + "_".join(map(str, mnk)))
        if (i < mnklen):
            sys.stdout.write([",\n", ", "][0 != (i % m)])
    print
    print "  };"
    print "  const int i = libxsmm_dispatch_index(m, n, k);"
    print "  return 0 <= i ? functions[i] : 0;"
    print "}"


def create_dispatch_bsearch(mnklist, generate_cpp):
    print "#include <libxsmm.h>"
    print
    print "#if defined(LIBXSMM_OFFLOAD)"
    print "# pragma offload_attribute(push,target(mic))"
    print "#endif"
    if (0 != generate_cpp):
        print "#include <algorithm>"
    print "#include <stdlib.h>"
    print "#if defined(LIBXSMM_OFFLOAD)"
    print "# pragma offload_attribute(pop)"
    print "#endif"
    print
    print
    print "LIBXSMM_TARGET(mic) int libxsmm_dispatch_compare3(const void* a, const void* b)"
    print "{"
    print "  const int *const ia = (const int*)a, *const ib = (const int*)b;"
    print "  const int d0 = ia[0] - ib[0], d1 = ia[1] - ib[1], d2 = ia[2] - ib[2];"
    print "  return 0 != d0 ? d0 : (0 != d1 ? d1 : d2);"
    print "}"
    print
    print
    if (0 != generate_cpp):
        print "LIBXSMM_TARGET(mic) bool libxsmm_dispatch_compare2(const int* a, const int* b)"
        print "{"
        print "  return 0 > libxsmm_dispatch_compare3(a, b);"
        print "}"
        print
        print
    print "LIBXSMM_TARGET(mic) int libxsmm_dispatch_index(int m, int n, int k)"
    print "{"
    print "  static /*const*/ int indices[] = {"
    sys.stdout.write("    ")
    i, m, mnklen = 0, 30, len(mnklist)
    for mnk in mnklist:
        i = i + 1
        sys.stdout.write(", ".join(map(str, mnk)))
        if (i < mnklen):
            sys.stdout.write([",\n", ", "][0 != (i % m)])
    print
    print "  };"
    if (0 != generate_cpp):
        print "  typedef int mnk_type[3];"
        print "  const mnk_type mnk = { m, n, k }, *const begin = reinterpret_cast<const mnk_type*>(indices), *const end = begin + " + str(mnklen) + ";"
        print "  const mnk_type *const hit = std::lower_bound(begin, end, mnk, libxsmm_dispatch_compare2);"
        print "  return (end != hit && 0 == libxsmm_dispatch_compare3(hit, mnk)) ? static_cast<int>(hit - begin) : -1;"
    else:
        print "  const int* hit = 0;"
        print "  int mnk[3];"
        print
        print "  mnk[0] = m; mnk[1] = n; mnk[2] = k;"
        print "  hit = (const int*)bsearch(mnk, indices, " + str(mnklen) + ", 3 * sizeof(*indices), libxsmm_dispatch_compare3);"
        print "  return 0 != hit ? ((int)(hit - indices) / 3) : -1;"
    print "}"
    print
    print
    create_dispatch_bsearch_function("s", mnklist)
    print
    print
    create_dispatch_bsearch_function("d", mnklist)


if __name__ == '__main__':
    argc = len(sys.argv)
    if (5 < argc):
        fileName, fileExtension = os.path.splitext(sys.argv[1])
        mnklist = libxsmm_utilities.load_mnklist(sys.argv[3:])
        threshold = int(sys.argv[2])
        maxmnk = libxsmm_utilities.max_mnk(mnklist, threshold)
        maxdim = int(maxmnk ** (1.0 / 3.0) + 0.5)
        maxm = libxsmm_utilities.max_mnk(mnklist, maxdim, 0)
        maxn = libxsmm_utilities.max_mnk(mnklist, maxdim, 1)
        maxk = libxsmm_utilities.max_mnk(mnklist, maxdim, 2)
        sparsity = int(sys.argv[3])
        if (1 < sparsity and (maxm * maxn * maxk) > (sparsity * maxmnk)):
            create_dispatch_bsearch(mnklist, ".c" != fileExtension)
        else:
            create_dispatch_direct(mnklist, maxmnk)
    else:
        raise ValueError(sys.argv[0] + ": wrong number of arguments!")
