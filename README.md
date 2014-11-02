LIBXSMM
=======
Library for small matrix-matrix multiplications targeting Intel Architecture (x86). This initial version of the library is targeting the Intel Xeon Phi coprocessor (an instance of the Intel Many Integrated Core Architecture "MIC") particularly using KNC intrinsic functions.

The code can be compiled to native code which is also usable in an offloaded code region (via a FORTRAN directive or via C/C++ preprocessor pragma). The prerequisite for offloading the code is to compile it to position-independent (PIC) code even when building a static library.

Performance: the presented code is by no means "optimal" or "best-performing" - it just uses Intrinsics. In fact, a well-optimizing compiler may produce better performing code.

Instructions
============
To compile the library run:

```sh
make
```

The interface is produced inside of the *include* directory. The library archives are produced inside of the *lib* directory with the *mic* subdirectory containing the native library and the *intel64* folder storing the hybrid archive containing host and MIC code.

To remove intermediate files use:

```sh
make clean
```

or to remove all generated files including the interface and library archive files:

```sh
make realclean
```

The usual `make install` is simply a shortcut for `make; make clean`.

The library can be configured to accept row-major (default) or column-major order matrices. Change the variable *ROW_MAJOR* inside of the Makefile (0 for column-major, and row-major order otherwise), or build the library in the following way to configure the column-major format:

```sh
make ROW_MAJOR=0
```

The interface of the library (include/xsmm_knc.h) defines the preprocessor symbols *LIBXSMM_ROW_MAJOR* and *LIBXSMM_COL_MAJOR* to mark the storage order the library was built for.

To perform the matrix-matrix multiplication C(M,N) = C(M,N) + A(M,K) \* B(K,N), one of the following two interfaces can be used:

```C
libxsmm_dmm_function libxsmm_dmm_dispatch(int M, int N, int K); // if non-zero call (*function)(M, N, K)
void dc_smm_dnn(int M, int N, int K, const double* A, const double* B, double* C); // automatic dispatch
```

To specialize for certain matrix sizes (M, N, and K values), one can adjust the variables inside of the Makefile or for example build in the following way:

```sh
make INDICES_M="2 4" INDICES_N="1" INDICES_K="$(echo $(seq 2 5))"
```

The above example generates the following (M,N,K) values:

```
(2,1,2), (2,1,3), (2,1,4), (2,1,5),
(4,1,2), (4,1,3), (4,1,4), (4,1,5)
```

More Details
============
The function *libxsmm_dmm_dispatch* helps to amortize the cost of the dispatch when multiple calls with the same M, N, and K are needed. The symbol *dc_smm_dnn* is actually a macro that allows to inline the dispatch logic; use *LIBXSMM_MM(double, ...)* in new code rather than *dc_smm_dnn(...)*. The code dispatch uses three levels:

1. Specialized routine,
2. Inlined code, and
3. BLAS library call.

The level 2 and 3 may be supplied by the Intel MKL 11.2 DIRECT CALL feature. Beside of the generic interface, one can call a specific kernel e.g., *libxsmm_dmm_4_4_4*.

Further, the preprocessor symbol *LIBXSMM_MAX_MNK* denotes the largest problem size (M x N x K) that belongs to level (1) and (2), and therefore determines if a matrix-matrix multiplication falls back to level (3) calling the BLAS library linked with LIBXSMM. This threshold can be configured using for example:

```sh
make THRESHOLD=$((24 * 24 * 24))
```

The maximum of the given threshold together with the largest requested specialization (according to *INDICES_M*, *INDICES_N*, and *INDICES_K*) defines the value of *LIBXSMM_MAX_MNK*.
