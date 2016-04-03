# Export all variables to sub-make processes.
#.EXPORT_ALL_VARIABLES: #export

# Automatically disable parallel builds
# depending on the version of GNU Make.
# MAKE_PARALLEL=0: disable explicitly
# MAKE_PARALLEL=1: enable explicitly
ifeq (0,$(MAKE_PARALLEL))
.NOTPARALLEL:
else ifeq (,$(strip $(MAKE_PARALLEL)))
ifneq (3.82,$(firstword $(sort $(MAKE_VERSION) 3.82)))
.NOTPARALLEL:
endif
endif

# Python interpreter
PYTHON ?= python

# THRESHOLD problem size (M x N x K) determining when to use BLAS; can be zero
THRESHOLD ?= $(shell echo $$((80 * 80 * 80)))

# Use ROW_MAJOR matrix representation if set to 1, COL_MAJOR otherwise
ROW_MAJOR ?= 0

# Generates M,N,K-combinations for each comma separated group e.g., "1, 2, 3" gnerates (1,1,1), (2,2,2),
# and (3,3,3). This way a heterogeneous set can be generated e.g., "1 2, 3" generates (1,1,1), (1,1,2),
# (1,2,1), (1,2,2), (2,1,1), (2,1,2) (2,2,1) out of the first group, and a (3,3,3) for the second group
# To generate a series of square matrices one can specify e.g., make MNK=$(echo $(seq -s, 1 5))
# Alternative to MNK, index sets can be specified separately according to a loop nest relationship
# (M(N(K))) using M, N, and K separately. Please consult the documentation for further details.
MNK ?= 0

# Enable thread-local cache of recently dispatched kernels either
# 0: "disable", 1: "enable", or small power-of-two number.
CACHE ?= 1

# Issue software prefetch instructions (see end of section
# https://github.com/hfp/libxsmm/#generator-driver)
# Use the enumerator 1...9, or the exact strategy
# name pfsigonly...AL2jpst_BL2viaC.
# 1: auto-select
# 2: pfsigonly
# 3: BL2viaC
# 4: AL2
# 5: curAL2
# 6: AL2_BL2viaC
# 7: curAL2_BL2viaC
# 8: AL2jpst
# 9: AL2jpst_BL2viaC
PREFETCH ?= 0

# Preferred precision when registering statically generated code versions
# 0: SP and DP code versions to be registered
# 1: SP only
# 2: DP only
PRECISION ?= 0

# Specify an alignment (Bytes)
ALIGNMENT ?= 64

# Generate code using aligned Load/Store instructions
# !=0: enable if lda/ldc (m) is a multiple of ALIGNMENT
# ==0: disable emitting aligned Load/Store instructions
ALIGNED_STORES ?= 0
ALIGNED_LOADS ?= 0

# Alpha argument of GEMM
# Supported: 1.0
ALPHA ?= 1
ifneq (1,$(ALPHA))
$(error ALPHA needs to be 1)
endif

# Beta argument of GEMM
# Supported: 0.0, 1.0
# 0: C  = A * B
# 1: C += A * B
BETA ?= 1
ifneq (0,$(BETA))
ifneq (1,$(BETA))
$(error BETA needs to be eiter 0 or 1)
endif
endif

ifneq (1,$(CACHE))
	DFLAGS = -DLIBXSMM_CACHESIZE=$(CACHE)
endif

ROOTDIR = $(abspath $(dir $(firstword $(MAKEFILE_LIST))))
SPLDIR = $(ROOTDIR)/samples
SCRDIR = $(ROOTDIR)/scripts
TSTDIR = $(ROOTDIR)/tests
SRCDIR = $(ROOTDIR)/src
INCDIR = include
BLDDIR = build
OUTDIR = lib
BINDIR = bin
DOCDIR = documentation

# subdirectories for prefix based installation
PINCDIR = $(INCDIR)
POUTDIR = $(OUTDIR)
PBINDIR = $(BINDIR)
PTSTDIR = tests
PDOCDIR = share/libxsmm

CXXFLAGS = $(NULL)
CFLAGS = $(NULL)
DFLAGS = -DLIBXSMM_BUILD
IFLAGS = -I$(INCDIR) -I$(BLDDIR) -I$(SRCDIR)

PTHREAD ?= 1
STATIC ?= 1

# JIT backend is enabled by default
JIT ?= 1
ifneq (0,$(JIT))
	AVX ?= 0
	SSE ?= 1
endif

# OpenMP is disabled by default and LIBXSMM is
# always agnostic wrt the threading runtime
OMP ?= 0

BLAS_WARNING ?= 0
ifeq (0,$(STATIC))
	ifeq (Windows_NT,$(OS))
		BLAS_WARNING = 1
		BLAS ?= 2
	else ifeq (Darwin,$(shell uname))
		BLAS_WARNING = 1
		BLAS ?= 2
	endif
endif

# include common Makefile artifacts
include $(ROOTDIR)/Makefile.inc

# Number of repeated calls (tests)
TESTSIZE ?= 1

ifeq (1,$(AVX))
	GENTARGET = snb
else ifeq (2,$(AVX))
	GENTARGET = hsw
else ifeq (3,$(AVX))
	GENTARGET = knl
else ifneq (0,$(SSE))
	GENTARGET = wsm
else
	GENTARGET = noarch
endif

ifeq (0,$(STATIC))
	GENERATOR = @$(ENV) \
		LD_LIBRARY_PATH="$(OUTDIR):$(LD_LIBRARY_PATH)" \
		PATH="$(OUTDIR):$(PATH)" \
	$(BINDIR)/libxsmm_gemm_generator
else
	GENERATOR = $(BINDIR)/libxsmm_gemm_generator
endif

INDICES ?= $(shell $(PYTHON) $(SCRDIR)/libxsmm_utilities.py -1 $(THRESHOLD) $(words $(MNK)) $(MNK) $(words $(M)) $(words $(N)) $(M) $(N) $(K))
NINDICES = $(words $(INDICES))

HEADERS = $(shell ls -1 $(SRCDIR)/*.h 2> /dev/null | tr "\n" " ") \
          $(SRCDIR)/libxsmm_gemm_extwrap.c \
          $(SRCDIR)/libxsmm_gemm_diff.c \
          $(SRCDIR)/libxsmm_cpuid_x86.c \
          $(SRCDIR)/libxsmm_hash.c \
          $(ROOTDIR)/include/libxsmm_frontend.h \
          $(ROOTDIR)/include/libxsmm_generator.h \
          $(ROOTDIR)/include/libxsmm_macros.h \
          $(ROOTDIR)/include/libxsmm_timer.h \
          $(ROOTDIR)/include/libxsmm_typedefs.h

SRCFILES_KERNELS = $(patsubst %,$(BLDDIR)/mm_%.c,$(INDICES))
SRCFILES_GEN_LIB = $(patsubst %,$(SRCDIR)/%,$(wildcard $(SRCDIR)/generator_*.c) libxsmm_timer.c libxsmm_trace.c)
SRCFILES_GEN_GEMM_BIN = $(patsubst %,$(SRCDIR)/%,libxsmm_generator_gemm_driver.c)
OBJFILES_GEN_LIB = $(patsubst %,$(BLDDIR)/%.o,$(basename $(notdir $(SRCFILES_GEN_LIB))))
OBJFILES_GEN_GEMM_BIN = $(patsubst %,$(BLDDIR)/%.o,$(basename $(notdir $(SRCFILES_GEN_GEMM_BIN))))
OBJFILES_HST = $(BLDDIR)/intel64/libxsmm.o $(BLDDIR)/intel64/libxsmm_gemm.o
OBJFILES_MIC = $(BLDDIR)/mic/libxsmm.o $(BLDDIR)/mic/libxsmm_gemm.o \
               $(BLDDIR)/mic/libxsmm_trace.o $(BLDDIR)/mic/libxsmm_timer.o
KERNELOBJS_HST = $(patsubst %,$(BLDDIR)/intel64/mm_%.o,$(INDICES))
KERNELOBJS_MIC = $(patsubst %,$(BLDDIR)/mic/mm_%.o,$(INDICES))
WRAPOBJS_HST = $(BLDDIR)/intel64/libxsmm_gemm_extwrap.o
WRAPOBJS_MIC = $(BLDDIR)/mic/libxsmm_gemm_extwrap.o
EXTOBJS_HST = $(BLDDIR)/intel64/libxsmm_gemm_extomp.o
EXTOBJS_MIC = $(BLDDIR)/mic/libxsmm_gemm_extomp.o

# list of object might be "incomplete" if not all code gen. FLAGS are supplied with clean target!
OBJECTS = $(OBJFILES_GEN_LIB) $(OBJFILES_GEN_GEMM_BIN) $(OBJFILES_HST) $(OBJFILES_MIC) \
          $(KERNELOBJS_HST) $(KERNELOBJS_MIC) $(WRAPOBJS_HST) $(WRAPOBJS_MIC) \
          $(EXTOBJS_HST) $(EXTOBJS_MIC)
FTNOBJS = $(BLDDIR)/intel64/libxsmm-mod.o $(BLDDIR)/mic/libxsmm-mod.o

.PHONY: libxsmm
libxsmm: lib

.PHONY: lib
lib: headers drytest lib_hst lib_mic

.PHONY: all
all: lib samples

.PHONY: headers
headers: cheader fheader

.PHONY: interface
interface: headers

.PHONY: lib_mic
lib_mic: clib_mic flib_mic ext_mic

.PHONY: lib_hst
lib_hst: clib_hst flib_hst ext_hst

PREFETCH_ID = 0
PREFETCH_SCHEME = nopf
PREFETCH_TYPE = 0

ifneq (0,$(shell echo $$((2 <= $(PREFETCH) && $(PREFETCH) <= 9))))
	PREFETCH_ID = $(PREFETCH)
else ifeq (1,$(PREFETCH)) # auto
	PREFETCH_ID = 1
else ifeq (pfsigonly,$(PREFETCH))
	PREFETCH_ID = 2
else ifeq (BL2viaC,$(PREFETCH))
	PREFETCH_ID = 3
else ifeq (AL2,$(PREFETCH))
	PREFETCH_ID = 4
else ifeq (curAL2,$(PREFETCH))
	PREFETCH_ID = 5
else ifeq (AL2_BL2viaC,$(PREFETCH))
	PREFETCH_ID = 6
else ifeq (curAL2_BL2viaC,$(PREFETCH))
	PREFETCH_ID = 7
else ifeq (AL2jpst,$(PREFETCH))
	PREFETCH_ID = 8
else ifeq (AL2jpst_BL2viaC,$(PREFETCH))
	PREFETCH_ID = 9
endif

# Mapping build options to libxsmm_prefetch_type (see include/libxsmm_typedefs.h)
ifeq (1,$(PREFETCH_ID))
	# Prefetch "auto" is a pseudo strategy introduced by the frontend;
	# select "pfsigonly" for statically generated code.
	PREFETCH_SCHEME = pfsigonly
	PREFETCH_TYPE = -1
	ifneq (0,$(MIC))
		ifneq (0,$(MPSS))
			PREFETCH_SCHEME_MIC = AL2_BL2viaC
		endif
	endif
else ifeq (2,$(PREFETCH_ID))
	PREFETCH_SCHEME = pfsigonly
	PREFETCH_TYPE = 1
else ifeq (3,$(PREFETCH_ID))
	PREFETCH_SCHEME = BL2viaC
	PREFETCH_TYPE = 8
else ifeq (4,$(PREFETCH_ID))
	PREFETCH_SCHEME = AL2
	PREFETCH_TYPE = 2
else ifeq (5,$(PREFETCH_ID))
	PREFETCH_SCHEME = curAL2
	PREFETCH_TYPE = 16
else ifeq (6,$(PREFETCH_ID))
	PREFETCH_SCHEME = AL2_BL2viaC
	PREFETCH_TYPE = $(shell echo $$((8 | 2)))
else ifeq (7,$(PREFETCH_ID))
	PREFETCH_SCHEME = curAL2_BL2viaC
	PREFETCH_TYPE = $(shell echo $$((8 | 16)))
else ifeq (8,$(PREFETCH_ID))
	PREFETCH_SCHEME = AL2jpst
	PREFETCH_TYPE = 4
else ifeq (9,$(PREFETCH_ID))
	PREFETCH_SCHEME = AL2jpst_BL2viaC
	PREFETCH_TYPE = $(shell echo $$((8 | 4)))
endif
ifeq (,$(PREFETCH_SCHEME_MIC))
	PREFETCH_SCHEME_MIC = PREFETCH_SCHEME
endif

# Mapping build options to libxsmm_gemm_flags (see include/libxsmm_typedefs.h)
FLAGS = $(shell echo $$((((0!=$(ALIGNED_LOADS))*4) | ((0!=$(ALIGNED_STORES))*8))))

SUPPRESS_UNUSED_VARIABLE_WARNINGS = LIBXSMM_UNUSED(A); LIBXSMM_UNUSED(B); LIBXSMM_UNUSED(C);
ifneq (nopf,$(PREFETCH_SCHEME))
	SUPPRESS_UNUSED_VARIABLE_WARNINGS += LIBXSMM_UNUSED(A_prefetch); LIBXSMM_UNUSED(B_prefetch);
	SUPPRESS_UNUSED_PREFETCH_WARNINGS = $(NULL)  LIBXSMM_UNUSED(C_prefetch);~
endif

.PHONY: cheader
cheader: $(INCDIR)/libxsmm.h
$(INCDIR)/libxsmm.h: .state $(INCDIR)/.make \
                     $(ROOTDIR)/Makefile $(ROOTDIR)/Makefile.inc \
                     $(ROOTDIR)/version.txt \
                     $(HEADERS)
	@if [ -e $(ROOTDIR)/.hooks/install.sh ]; then \
		$(ROOTDIR)/.hooks/install.sh ; \
	fi
	@cp $(ROOTDIR)/include/libxsmm_macros.h $(INCDIR) 2> /dev/null || true
	@cp $(ROOTDIR)/include/libxsmm_typedefs.h $(INCDIR) 2> /dev/null || true
	@cp $(ROOTDIR)/include/libxsmm_frontend.h $(INCDIR) 2> /dev/null || true
	@cp $(ROOTDIR)/include/libxsmm_generator.h $(INCDIR) 2> /dev/null || true
	@cp $(ROOTDIR)/include/libxsmm_timer.h $(INCDIR) 2> /dev/null || true
	@$(PYTHON) $(SCRDIR)/libxsmm_interface.py $(SRCDIR)/libxsmm.template.h \
		$(PRECISION) $(MAKE_ILP64) $(OFFLOAD) $(ALIGNMENT) $(ROW_MAJOR) $(PREFETCH_TYPE) \
		$(shell echo $$((0<$(THRESHOLD)?$(THRESHOLD):0))) $(shell echo $$(($(PTHREAD)+$(OMP)))) \
		$(JIT) $(FLAGS) $(ALPHA) $(BETA) $(INDICES) > $@
	$(info ================================================================================)
	$(info $(INFO))
	$(info ================================================================================)
ifeq (,$(strip $(FC)))
ifeq (,$(strip $(FC_VERSION_STRING)))
	$(info Fortran Compiler is missing: building without Fortran support!)
else
	$(info Fortran Compiler $(FC_VERSION_STRING) is outdated!)
endif
	$(info ================================================================================)
endif
ifeq (0,$(STATIC))
ifeq (Windows_NT,$(UNAME))
	$(info The shared link-time wrapper (libxsmmext) is not supported under Windows/Cygwin!)
	$(info ================================================================================)
endif
endif
ifneq (0,$(BLAS_WARNING))
	$(info Building a shared library requires to link against BLAS since there is)
	$(info no runtime resolution/search for weak symbols implemented for this OS.)
endif
ifneq (0,$(BLAS))
ifeq (0,$(BLAS_WARNING))
	$(info LIBXSMM is link-time agnostic with respect to BLAS/GEMM!)
	$(info Linking a certain BLAS library may prevent users to decide.)
endif
ifeq (1,$(BLAS))
	$(info LIBXSMM's THRESHOLD already prevents calling small GEMMs!)
	$(info A sequential BLAS is superfluous with respect to LIBXSMM.)
endif
	$(info ================================================================================)
endif

.PHONY: fheader
fheader: $(INCDIR)/libxsmm.f
$(INCDIR)/libxsmm.f: .state $(INCDIR)/.make $(BLDDIR)/.make \
                     $(SRCDIR)/libxsmm.template.f $(ROOTDIR)/.hooks/install.sh $(ROOTDIR)/version.txt \
                     $(SCRDIR)/libxsmm_interface.py $(SCRDIR)/libxsmm_utilities.py \
                     $(ROOTDIR)/Makefile $(ROOTDIR)/Makefile.inc
	@$(ROOTDIR)/.hooks/install.sh
ifeq (0,$(OFFLOAD))
	@$(PYTHON) $(SCRDIR)/libxsmm_interface.py $(SRCDIR)/libxsmm.template.f \
		$(PRECISION) $(MAKE_ILP64) $(OFFLOAD) $(ALIGNMENT) $(ROW_MAJOR) $(PREFETCH_TYPE) \
		$(shell echo $$((0<$(THRESHOLD)?$(THRESHOLD):0))) $(shell echo $$(($(PTHREAD)+$(OMP)))) \
		$(JIT) $(FLAGS) $(ALPHA) $(BETA) $(INDICES) | \
	sed '/ATTRIBUTES OFFLOAD:MIC/d' > $@
else
	@$(PYTHON) $(SCRDIR)/libxsmm_interface.py $(SRCDIR)/libxsmm.template.f \
		$(PRECISION) $(MAKE_ILP64) $(OFFLOAD) $(ALIGNMENT) $(ROW_MAJOR) $(PREFETCH_TYPE) \
		$(shell echo $$((0<$(THRESHOLD)?$(THRESHOLD):0))) $(shell echo $$(($(PTHREAD)+$(OMP)))) \
		$(JIT) $(FLAGS) $(ALPHA) $(BETA) $(INDICES) > $@
endif

.PHONY: sources
sources: $(SRCFILES_KERNELS) $(BLDDIR)/libxsmm_dispatch.h
$(BLDDIR)/libxsmm_dispatch.h: $(BLDDIR)/.make $(SCRDIR)/libxsmm_dispatch.py $(SRCFILES_KERNELS) $(INCDIR)/libxsmm.h
	@$(PYTHON) $(SCRDIR)/libxsmm_dispatch.py $(PRECISION) $(THRESHOLD) $(INDICES) > $@

$(BLDDIR)/%.c: $(BLDDIR)/.make $(INCDIR)/libxsmm.h $(BINDIR)/libxsmm_gemm_generator $(SCRDIR)/libxsmm_utilities.py $(SCRDIR)/libxsmm_specialized.py
ifneq (,$(strip $(SRCFILES_KERNELS)))
	$(eval MVALUE := $(shell echo $(basename $@) | cut -d_ -f2))
	$(eval NVALUE := $(shell echo $(basename $@) | cut -d_ -f3))
	$(eval KVALUE := $(shell echo $(basename $@) | cut -d_ -f4))
ifneq (0,$(ROW_MAJOR)) # row-major
	$(eval MNVALUE := $(NVALUE))
	$(eval NMVALUE := $(MVALUE))
else # column-major
	$(eval MNVALUE := $(MVALUE))
	$(eval NMVALUE := $(NVALUE))
endif
	$(eval ASTSP := $(shell echo $$((0!=$(ALIGNED_STORES)&&0==($(MNVALUE)*4)%$(ALIGNMENT)))))
	$(eval ASTDP := $(shell echo $$((0!=$(ALIGNED_STORES)&&0==($(MNVALUE)*8)%$(ALIGNMENT)))))
	$(eval ALDSP := $(shell echo $$((0!=$(ALIGNED_LOADS)&&0==($(MNVALUE)*4)%$(ALIGNMENT)))))
	$(eval ALDDP := $(shell echo $$((0!=$(ALIGNED_LOADS)&&0==($(MNVALUE)*8)%$(ALIGNMENT)))))
	@echo "#include <libxsmm.h>" > $@
	@echo >> $@
ifneq (0,$(MIC))
ifneq (0,$(MPSS))
ifneq (2,$(PRECISION))
	@echo "#define LIBXSMM_GENTARGET_knc_sp" >> $@
endif
ifneq (1,$(PRECISION))
	@echo "#define LIBXSMM_GENTARGET_knc_dp" >> $@
endif
endif
endif
ifeq (noarch,$(GENTARGET))
ifneq (2,$(PRECISION))
	@echo "#define LIBXSMM_GENTARGET_knl_sp" >> $@
	@echo "#define LIBXSMM_GENTARGET_hsw_sp" >> $@
	@echo "#define LIBXSMM_GENTARGET_snb_sp" >> $@
	@echo "#define LIBXSMM_GENTARGET_wsm_sp" >> $@
endif
ifneq (1,$(PRECISION))
	@echo "#define LIBXSMM_GENTARGET_knl_dp" >> $@
	@echo "#define LIBXSMM_GENTARGET_hsw_dp" >> $@
	@echo "#define LIBXSMM_GENTARGET_snb_dp" >> $@
	@echo "#define LIBXSMM_GENTARGET_wsm_dp" >> $@
endif
	@echo >> $@
	@echo >> $@
ifneq (2,$(PRECISION))
	$(GENERATOR) dense $@ libxsmm_s$(basename $(notdir $@))_knl $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDSP) $(ASTSP) knl $(PREFETCH_SCHEME) SP
	$(GENERATOR) dense $@ libxsmm_s$(basename $(notdir $@))_hsw $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDSP) $(ASTSP) hsw $(PREFETCH_SCHEME) SP
	$(GENERATOR) dense $@ libxsmm_s$(basename $(notdir $@))_snb $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDSP) $(ASTSP) snb $(PREFETCH_SCHEME) SP
	$(GENERATOR) dense $@ libxsmm_s$(basename $(notdir $@))_wsm $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDSP) $(ASTSP) wsm $(PREFETCH_SCHEME) SP
endif
ifneq (1,$(PRECISION))
	$(GENERATOR) dense $@ libxsmm_d$(basename $(notdir $@))_knl $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDDP) $(ASTDP) knl $(PREFETCH_SCHEME) DP
	$(GENERATOR) dense $@ libxsmm_d$(basename $(notdir $@))_hsw $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDDP) $(ASTDP) hsw $(PREFETCH_SCHEME) DP
	$(GENERATOR) dense $@ libxsmm_d$(basename $(notdir $@))_snb $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDDP) $(ASTDP) snb $(PREFETCH_SCHEME) DP
	$(GENERATOR) dense $@ libxsmm_d$(basename $(notdir $@))_wsm $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDDP) $(ASTDP) wsm $(PREFETCH_SCHEME) DP
endif
else
ifneq (2,$(PRECISION))
	@echo "#define LIBXSMM_GENTARGET_$(GENTARGET)_sp" >> $@
endif
ifneq (1,$(PRECISION))
	@echo "#define LIBXSMM_GENTARGET_$(GENTARGET)_dp" >> $@
endif
	@echo >> $@
	@echo >> $@
ifneq (2,$(PRECISION))
	$(GENERATOR) dense $@ libxsmm_s$(basename $(notdir $@))_$(GENTARGET) $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDSP) $(ASTSP) $(GENTARGET) $(PREFETCH_SCHEME) SP
endif
ifneq (1,$(PRECISION))
	$(GENERATOR) dense $@ libxsmm_d$(basename $(notdir $@))_$(GENTARGET) $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDDP) $(ASTDP) $(GENTARGET) $(PREFETCH_SCHEME) DP
endif
endif
ifneq (0,$(MIC))
ifneq (0,$(MPSS))
ifneq (2,$(PRECISION))
	$(GENERATOR) dense $@ libxsmm_s$(basename $(notdir $@))_knc $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDSP) $(ASTDP) knc $(PREFETCH_SCHEME_MIC) SP
endif
ifneq (1,$(PRECISION))
	$(GENERATOR) dense $@ libxsmm_d$(basename $(notdir $@))_knc $(MNVALUE) $(NMVALUE) $(KVALUE) $(MNVALUE) $(KVALUE) $(MNVALUE) $(ALPHA) $(BETA) $(ALDSP) $(ASTDP) knc $(PREFETCH_SCHEME_MIC) DP
endif
endif
endif
	$(eval TMPFILE = $(shell mktemp /tmp/fileXXXXXX))
	@cat $@ | sed \
		-e 's/void libxsmm_/LIBXSMM_INLINE LIBXSMM_RETARGETABLE void libxsmm_/' \
		-e 's/#ifndef NDEBUG/$(SUPPRESS_UNUSED_PREFETCH_WARNINGS)#ifdef LIBXSMM_NEVER_DEFINED/' \
		-e 's/#pragma message (".*KERNEL COMPILATION ERROR in: " __FILE__)/  $(SUPPRESS_UNUSED_VARIABLE_WARNINGS)/' \
		-e '/#error No kernel was compiled, lacking support for current architecture?/d' \
		-e '/#pragma message (".*KERNEL COMPILATION WARNING: compiling ..* code on ..* or newer architecture: " __FILE__)/d' \
		| tr '~' '\n' > $(TMPFILE)
	@$(PYTHON) $(SCRDIR)/libxsmm_specialized.py $(PRECISION) $(MVALUE) $(NVALUE) $(KVALUE) $(PREFETCH_TYPE) >> $(TMPFILE)
	@mv $(TMPFILE) $@
endif

define DEFINE_COMPILE_RULE
$(1): $(2) $(3) $(dir $(1))/.make
	$(CC) $(CFLAGS) $(DFLAGS) $(IFLAGS) $(4) -c $(2) -o $(1)
endef

ifeq (0,$(OMP))
EXTOMPFLAG = $(OMPFLAG)
endif

ifneq (0,$(MIC))
ifneq (0,$(MPSS))
$(foreach OBJ,$(OBJFILES_MIC),$(eval $(call DEFINE_COMPILE_RULE, \
	$(OBJ), $(patsubst %.o,$(SRCDIR)/%.c,$(notdir $(OBJ))), \
	$(INCDIR)/libxsmm.h $(BLDDIR)/libxsmm_dispatch.h, \
	-mmic)))
$(foreach OBJ,$(KERNELOBJS_MIC),$(eval $(call DEFINE_COMPILE_RULE, \
	$(OBJ), $(patsubst %.o,$(BLDDIR)/%.c,$(notdir $(OBJ))), \
	$(INCDIR)/libxsmm.h, \
	-mmic)))
$(foreach OBJ,$(EXTOBJS_MIC),$(eval $(call DEFINE_COMPILE_RULE, \
	$(OBJ), $(patsubst %.o,$(SRCDIR)/%.c,$(notdir $(OBJ))), \
	$(INCDIR)/libxsmm.h, \
	-mmic $(EXTOMPFLAG))))
ifeq (0,$(STATIC))
$(foreach OBJ,$(WRAPOBJS_MIC),$(eval $(call DEFINE_COMPILE_RULE, \
	$(OBJ), $(patsubst %.o,$(SRCDIR)/%.c,$(notdir $(OBJ))), \
	$(INCDIR)/libxsmm.h, \
	-mmic)))
endif
endif
endif

$(foreach OBJ,$(OBJFILES_HST),$(eval $(call DEFINE_COMPILE_RULE, \
	$(OBJ),$(patsubst %.o,$(SRCDIR)/%.c,$(notdir $(OBJ))), \
	$(INCDIR)/libxsmm.h $(BLDDIR)/libxsmm_dispatch.h, $(TARGET))))
$(foreach OBJ,$(KERNELOBJS_HST),$(eval $(call DEFINE_COMPILE_RULE, \
	$(OBJ),$(patsubst %.o,$(BLDDIR)/%.c,$(notdir $(OBJ))), \
	$(INCDIR)/libxsmm.h, $(TARGET))))
$(foreach OBJ,$(EXTOBJS_HST),$(eval $(call DEFINE_COMPILE_RULE, \
	$(OBJ),$(patsubst %.o,$(SRCDIR)/%.c,$(notdir $(OBJ))), \
	$(INCDIR)/libxsmm.h, $(TARGET) $(EXTOMPFLAG))))
$(foreach OBJ,$(OBJFILES_GEN_LIB),$(eval $(call DEFINE_COMPILE_RULE, \
	$(OBJ),$(patsubst %.o,$(SRCDIR)/%.c,$(notdir $(OBJ))), \
	$(INCDIR)/libxsmm.h, $(NULL))))
$(foreach OBJ,$(OBJFILES_GEN_GEMM_BIN),$(eval $(call DEFINE_COMPILE_RULE, \
	$(OBJ),$(patsubst %.o,$(SRCDIR)/%.c,$(notdir $(OBJ))), \
	$(INCDIR)/libxsmm.h, $(NULL))))

ifeq (0,$(STATIC))
$(foreach OBJ,$(WRAPOBJS_HST),$(eval $(call DEFINE_COMPILE_RULE, \
	$(OBJ),$(patsubst %.o,$(SRCDIR)/%.c,$(notdir $(OBJ))), \
	$(INCDIR)/libxsmm.h, $(TARGET))))
endif

.PHONY: compile_mic
ifneq (0,$(MIC))
ifneq (0,$(MPSS))
compile_mic:
$(BLDDIR)/mic/%.o: $(BLDDIR)/%.c $(BLDDIR)/mic/.make $(INCDIR)/libxsmm.h $(BLDDIR)/libxsmm_dispatch.h
	$(CC) $(CFLAGS) $(DFLAGS) $(IFLAGS) -mmic -c $< -o $@
endif
endif

.PHONY: compile_hst
compile_hst:
$(BLDDIR)/intel64/%.o: $(BLDDIR)/%.c $(BLDDIR)/intel64/.make $(INCDIR)/libxsmm.h $(BLDDIR)/libxsmm_dispatch.h
	$(CC) $(CFLAGS) $(DFLAGS) $(IFLAGS) $(TARGET) -c $< -o $@

.PHONY: module_mic
ifneq (0,$(MIC))
ifneq (0,$(MPSS))
ifneq (,$(strip $(FC)))
module_mic: $(BLDDIR)/mic/libxsmm-mod.o
$(BLDDIR)/mic/libxsmm-mod.o: $(BLDDIR)/mic/.make $(INCDIR)/mic/.make $(INCDIR)/libxsmm.f
	$(FC) $(FCMTFLAGS) $(FCFLAGS) $(DFLAGS) $(IFLAGS) -mmic -c $(INCDIR)/libxsmm.f -o $(BLDDIR)/mic/libxsmm-mod.o $(FMFLAGS) $(INCDIR)/mic
endif
endif
endif

.PHONY: module_hst
ifneq (,$(strip $(FC)))
module_hst: $(BLDDIR)/intel64/libxsmm-mod.o
$(BLDDIR)/intel64/libxsmm-mod.o: $(BLDDIR)/intel64/.make $(INCDIR)/libxsmm.f
	$(FC) $(FCMTFLAGS) $(FCFLAGS) $(DFLAGS) $(IFLAGS) $(TARGET) -c $(INCDIR)/libxsmm.f -o $(BLDDIR)/intel64/libxsmm-mod.o $(FMFLAGS) $(INCDIR)
endif

.PHONY: module
module: module_hst module_mic

.PHONY: build_generator_lib
build_generator_lib: $(OUTDIR)/libxsmmgen.$(LIBEXT)
$(OUTDIR)/libxsmmgen.$(LIBEXT): $(OUTDIR)/.make $(OBJFILES_GEN_LIB)
ifeq (0,$(STATIC))
	$(LD) -o $@ $(OBJFILES_GEN_LIB) -shared $(LDFLAGS) $(CLDFLAGS)
else
	$(AR) -rs $@ $(OBJFILES_GEN_LIB)
endif

.PHONY: generator
generator: $(BINDIR)/libxsmm_gemm_generator
$(BINDIR)/libxsmm_gemm_generator: $(BINDIR)/.make $(OBJFILES_GEN_GEMM_BIN) $(OUTDIR)/libxsmmgen.$(LIBEXT)
	$(CC) $(OBJFILES_GEN_GEMM_BIN) $(call libdir,$(OUTDIR)/libxsmmgen.$(LIBEXT)) $(LDFLAGS) $(CLDFLAGS) -o $@

.PHONY: clib_mic
ifneq (0,$(MIC))
ifneq (0,$(MPSS))
clib_mic: $(OUTDIR)/mic/libxsmm.$(LIBEXT)
$(OUTDIR)/mic/libxsmm.$(LIBEXT): $(OUTDIR)/mic/.make $(OBJFILES_MIC) $(KERNELOBJS_MIC)
ifeq (0,$(STATIC))
	$(LD) -o $@ $(OBJFILES_MIC) $(KERNELOBJS_MIC) -mmic -shared $(LDFLAGS) $(CLDFLAGS)
else
	$(AR) -rs $@ $(OBJFILES_MIC) $(KERNELOBJS_MIC)
endif
endif
endif

.PHONY: clib_hst
clib_hst: $(OUTDIR)/libxsmm.$(LIBEXT)
$(OUTDIR)/libxsmm.$(LIBEXT): $(OUTDIR)/.make $(OBJFILES_HST) $(OBJFILES_GEN_LIB) $(KERNELOBJS_HST)
ifeq (0,$(STATIC))
	$(LD) -o $@ $(OBJFILES_HST) $(OBJFILES_GEN_LIB) $(KERNELOBJS_HST) -shared $(LDFLAGS) $(CLDFLAGS)
else
	$(AR) -rs $@ $(OBJFILES_HST) $(OBJFILES_GEN_LIB) $(KERNELOBJS_HST)
endif

.PHONY: flib_mic
ifneq (0,$(MIC))
ifneq (0,$(MPSS))
ifneq (,$(strip $(FC)))
flib_mic: $(OUTDIR)/mic/libxsmmf.$(LIBEXT)
ifeq (0,$(STATIC))
$(OUTDIR)/mic/libxsmmf.$(LIBEXT): $(BLDDIR)/mic/libxsmm-mod.o $(OUTDIR)/mic/libxsmm.$(LIBEXT)
	$(FC) -o $@ $(BLDDIR)/mic/libxsmm-mod.o $(call libdir,$(OUTDIR)/mic/libxsmm.$(LIBEXT)) -mmic -shared $(FCMTFLAGS) $(LDFLAGS) $(FLDFLAGS) $(ELDFLAGS)
else
$(OUTDIR)/mic/libxsmmf.$(LIBEXT): $(BLDDIR)/mic/libxsmm-mod.o $(OUTDIR)/mic/.make
	$(AR) -rs $@ $(BLDDIR)/mic/libxsmm-mod.o
endif
endif
endif
endif

.PHONY: flib_hst
ifneq (,$(strip $(FC)))
flib_hst: $(OUTDIR)/libxsmmf.$(LIBEXT)
ifeq (0,$(STATIC))
$(OUTDIR)/libxsmmf.$(LIBEXT): $(BLDDIR)/intel64/libxsmm-mod.o $(OUTDIR)/libxsmm.$(LIBEXT)
	$(FC) -o $@ $(BLDDIR)/intel64/libxsmm-mod.o $(call libdir,$(OUTDIR)/libxsmm.$(LIBEXT)) -shared $(FCMTFLAGS) $(LDFLAGS) $(FLDFLAGS) $(ELDFLAGS)
else
$(OUTDIR)/libxsmmf.$(LIBEXT): $(BLDDIR)/intel64/libxsmm-mod.o $(OUTDIR)/.make
	$(AR) -rs $@ $(BLDDIR)/intel64/libxsmm-mod.o
endif
endif

.PHONY: ext_mic
ifneq (0,$(MIC))
ifneq (0,$(MPSS))
ext_mic: $(OUTDIR)/mic/libxsmmext.$(LIBEXT)
ifeq (0,$(STATIC))
$(OUTDIR)/mic/libxsmmext.$(LIBEXT): $(OUTDIR)/mic/.make $(EXTOBJS_MIC) $(WRAPOBJS_MIC) $(OUTDIR)/mic/libxsmm.$(DLIBEXT)
	$(LD) -o $@ $(EXTOBJS_MIC) $(WRAPOBJS_MIC) $(call libdir,$(OUTDIR)/mic/libxsmm.$(DLIBEXT)) -mmic -shared $(EXTOMPFLAG) $(LDFLAGS) $(CLDFLAGS)
else
$(OUTDIR)/mic/libxsmmext.$(LIBEXT): $(OUTDIR)/mic/.make $(EXTOBJS_MIC)
	$(AR) -rs $@ $(EXTOBJS_MIC)
endif
endif
endif

.PHONY: ext_hst
ext_hst: $(OUTDIR)/libxsmmext.$(LIBEXT)
ifeq (0,$(STATIC))
$(OUTDIR)/libxsmmext.$(LIBEXT): $(OUTDIR)/.make $(EXTOBJS_HST) $(WRAPOBJS_HST) $(OUTDIR)/libxsmm.$(DLIBEXT)
ifneq (Darwin,$(UNAME))
	$(LD) -o $@ $(EXTOBJS_HST) $(WRAPOBJS_HST) $(call libdir,$(OUTDIR)/libxsmm.$(DLIBEXT)) -shared $(EXTOMPFLAG) $(LDFLAGS) $(CLDFLAGS)
else
	$(LD) -o $@ $(EXTOBJS_HST) $(WRAPOBJS_HST) $(call libdir,$(OUTDIR)/libxsmm.$(DLIBEXT)) -shared $(LDFLAGS) $(CLDFLAGS)
endif
else # static
$(OUTDIR)/libxsmmext.$(LIBEXT): $(OUTDIR)/.make $(EXTOBJS_HST)
	$(AR) -rs $@ $(EXTOBJS_HST)
endif

.PHONY: samples
samples: cp2k dgemm nek smm

.PHONY: cp2k
cp2k: lib_hst
	@cd $(SPLDIR)/cp2k && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS)

.PHONY: cp2k_mic
cp2k_mic: lib_mic
	@cd $(SPLDIR)/cp2k && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) MIC=1 TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS)

.PHONY: dgemm
dgemm: lib_hst
	@cd $(SPLDIR)/dgemm && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=0 \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS)

.PHONY: dgemm_mic
dgemm_mic: lib_mic
	@cd $(SPLDIR)/dgemm && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) MIC=1 TRACE=0 \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS)

.PHONY: nek
nek: lib_hst
	@cd $(SPLDIR)/nek && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS)

.PHONY: nek_mic
nek_mic: lib_mic
	@cd $(SPLDIR)/nek && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) MIC=1 TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS)

.PHONY: smm
smm: lib_hst
	@cd $(SPLDIR)/smm && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS)

.PHONY: smm_mic
smm_mic: lib_mic
	@cd $(SPLDIR)/smm && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) MIC=1 TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS)

.PHONY: drytest
drytest: $(SPLDIR)/cp2k/cp2k-perf.sh $(SPLDIR)/smm/smmf-perf.sh \
	$(SPLDIR)/nek/axhm-perf.sh $(SPLDIR)/nek/grad-perf.sh $(SPLDIR)/nek/rstr-perf.sh

$(SPLDIR)/cp2k/cp2k-perf.sh: $(SPLDIR)/cp2k/.make $(ROOTDIR)/Makefile
	@echo "#!/bin/sh" > $@
	@echo >> $@
	@echo "HERE=\$$(cd \$$(dirname \$$0); pwd -P)" >> $@
	@echo "ECHO=\$$(which echo)" >> $@
	@echo "FILE=cp2k-perf.txt" >> $@
ifneq (,$(strip $(INDICES)))
	@echo "RUNS=\"$(INDICES)\"" >> $@
else
	@echo "RUNS=\"23_23_23 4_6_9 13_5_7 24_3_36\"" >> $@
endif
	@echo >> $@
	@echo "if [ \"\" != \"\$$1\" ]; then" >> $@
	@echo "  FILE=\$$1" >> $@
	@echo "  shift" >> $@
	@echo "fi" >> $@
	@echo "if [ \"\" != \"\$$1\" ]; then" >> $@
	@echo "  SIZE=\$$1" >> $@
	@echo "  shift" >> $@
	@echo "else" >> $@
	@echo "  SIZE=0" >> $@
	@echo "fi" >> $@
	@echo "cat /dev/null > \$${FILE}" >> $@
	@echo >> $@
	@echo "NRUN=1" >> $@
	@echo "NMAX=\$$(\$${ECHO} \$${RUNS} | wc -w)" >> $@
	@echo "for RUN in \$${RUNS} ; do" >> $@
	@echo "  MVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f1)" >> $@
	@echo "  NVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f2)" >> $@
	@echo "  KVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f3)" >> $@
	@echo "  >&2 \$\$${ECHO} -n \"\$${NRUN} of \$${NMAX} (M=\$${MVALUE} N=\$${NVALUE} K=\$${KVALUE})... \"" >> $@
	@echo "  ERROR=\$$({ CHECK=1 \$${HERE}/cp2k.sh \$${MVALUE} \$${SIZE} 0 \$${NVALUE} \$${KVALUE} >> \$${FILE}; } 2>&1)" >> $@
	@echo "  RESULT=\$$?" >> $@
	@echo "  if [ 0 != \$${RESULT} ]; then" >> $@
	@echo "    \$${ECHO} \"FAILED(\$${RESULT}) \$${ERROR}\"" >> $@
	@echo "    exit 1" >> $@
	@echo "  else" >> $@
	@echo "    \$${ECHO} \"OK \$${ERROR}\"" >> $@
	@echo "  fi" >> $@
	@echo "  \$${ECHO} >> \$${FILE}" >> $@
	@echo "  NRUN=\$$((NRUN+1))" >> $@
	@echo "done" >> $@
	@echo >> $@
	@chmod +x $@

$(SPLDIR)/smm/smmf-perf.sh: $(SPLDIR)/smm/.make $(ROOTDIR)/Makefile
	@echo "#!/bin/sh" > $@
	@echo >> $@
	@echo "HERE=\$$(cd \$$(dirname \$$0); pwd -P)" >> $@
	@echo "ECHO=\$$(which echo)" >> $@
	@echo "FILE=\$${HERE}/smmf-perf.txt" >> $@
ifneq (,$(strip $(INDICES)))
	@echo "RUNS=\"$(INDICES)\"" >> $@
else
	@echo "RUNS=\"23_23_23 4_6_9 13_5_7 24_3_36\"" >> $@
endif
	@echo >> $@
	@echo "if [ \"\" != \"\$$1\" ]; then" >> $@
	@echo "  FILE=\$$1" >> $@
	@echo "  shift" >> $@
	@echo "fi" >> $@
	@echo "cat /dev/null > \$${FILE}" >> $@
	@echo >> $@
	@echo "NRUN=1" >> $@
	@echo "NMAX=\$$(\$${ECHO} \$${RUNS} | wc -w)" >> $@
	@echo "for RUN in \$${RUNS} ; do" >> $@
	@echo "  MVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f1)" >> $@
	@echo "  NVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f2)" >> $@
	@echo "  KVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f3)" >> $@
	@echo "  >&2 \$\$${ECHO} -n \"\$${NRUN} of \$${NMAX} (M=\$${MVALUE} N=\$${NVALUE} K=\$${KVALUE})... \"" >> $@
	@echo "  ERROR=\$$({ CHECK=1 \$${HERE}/smm.sh \$${MVALUE} \$${NVALUE} \$${KVALUE} \$$* >> \$${FILE}; } 2>&1)" >> $@
	@echo "  RESULT=\$$?" >> $@
	@echo "  if [ 0 != \$${RESULT} ]; then" >> $@
	@echo "    \$${ECHO} \"FAILED(\$${RESULT}) \$${ERROR}\"" >> $@
	@echo "    exit 1" >> $@
	@echo "  else" >> $@
	@echo "    \$${ECHO} \"OK \$${ERROR}\"" >> $@
	@echo "  fi" >> $@
	@echo "  \$${ECHO} >> \$${FILE}" >> $@
	@echo "  NRUN=\$$((NRUN+1))" >> $@
	@echo "done" >> $@
	@echo >> $@
	@chmod +x $@

$(SPLDIR)/nek/axhm-perf.sh: $(SPLDIR)/nek/.make $(ROOTDIR)/Makefile
	@echo "#!/bin/sh" > $@
	@echo >> $@
	@echo "HERE=\$$(cd \$$(dirname \$$0); pwd -P)" >> $@
	@echo "ECHO=\$$(which echo)" >> $@
	@echo "FILE=\$${HERE}/axhm-perf.txt" >> $@
ifneq (,$(strip $(INDICES)))
	@echo "RUNS=\"$(INDICES)\"" >> $@
else
	@echo "RUNS=\"4_6_9 8_8_8 13_13_13 16_8_13\"" >> $@
endif
	@echo >> $@
	@echo "if [ \"\" != \"\$$1\" ]; then" >> $@
	@echo "  FILE=\$$1" >> $@
	@echo "  shift" >> $@
	@echo "fi" >> $@
	@echo "cat /dev/null > \$${FILE}" >> $@
	@echo >> $@
	@echo "NRUN=1" >> $@
	@echo "NMAX=\$$(\$${ECHO} \$${RUNS} | wc -w)" >> $@
	@echo "for RUN in \$${RUNS} ; do" >> $@
	@echo "  MVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f1)" >> $@
	@echo "  NVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f2)" >> $@
	@echo "  KVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f3)" >> $@
	@echo "  >&2 \$\$${ECHO} -n \"\$${NRUN} of \$${NMAX} (M=\$${MVALUE} N=\$${NVALUE} K=\$${KVALUE})... \"" >> $@
	@echo "  ERROR=\$$({ CHECK=1 \$${HERE}/axhm.sh \$${MVALUE} \$${NVALUE} \$${KVALUE} \$$* >> \$${FILE}; } 2>&1)" >> $@
	@echo "  RESULT=\$$?" >> $@
	@echo "  if [ 0 != \$${RESULT} ]; then" >> $@
	@echo "    \$${ECHO} \"FAILED(\$${RESULT}) \$${ERROR}\"" >> $@
	@echo "    exit 1" >> $@
	@echo "  else" >> $@
	@echo "    \$${ECHO} \"OK \$${ERROR}\"" >> $@
	@echo "  fi" >> $@
	@echo "  \$${ECHO} >> \$${FILE}" >> $@
	@echo "  NRUN=\$$((NRUN+1))" >> $@
	@echo "done" >> $@
	@echo >> $@
	@chmod +x $@

$(SPLDIR)/nek/grad-perf.sh: $(SPLDIR)/nek/.make $(ROOTDIR)/Makefile
	@echo "#!/bin/sh" > $@
	@echo >> $@
	@echo "HERE=\$$(cd \$$(dirname \$$0); pwd -P)" >> $@
	@echo "ECHO=\$$(which echo)" >> $@
	@echo "FILE=\$${HERE}/grad-perf.txt" >> $@
ifneq (,$(strip $(INDICES)))
	@echo "RUNS=\"$(INDICES)\"" >> $@
else
	@echo "RUNS=\"4_6_9 8_8_8 13_13_13 16_8_13\"" >> $@
endif
	@echo >> $@
	@echo "if [ \"\" != \"\$$1\" ]; then" >> $@
	@echo "  FILE=\$$1" >> $@
	@echo "  shift" >> $@
	@echo "fi" >> $@
	@echo "cat /dev/null > \$${FILE}" >> $@
	@echo >> $@
	@echo "NRUN=1" >> $@
	@echo "NMAX=\$$(\$${ECHO} \$${RUNS} | wc -w)" >> $@
	@echo "for RUN in \$${RUNS} ; do" >> $@
	@echo "  MVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f1)" >> $@
	@echo "  NVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f2)" >> $@
	@echo "  KVALUE=\$$(\$${ECHO} \$${RUN} | cut -d_ -f3)" >> $@
	@echo "  >&2 \$\$${ECHO} -n \"\$${NRUN} of \$${NMAX} (M=\$${MVALUE} N=\$${NVALUE} K=\$${KVALUE})... \"" >> $@
	@echo "  ERROR=\$$({ CHECK=1 \$${HERE}/grad.sh \$${MVALUE} \$${NVALUE} \$${KVALUE} \$$* >> \$${FILE}; } 2>&1)" >> $@
	@echo "  RESULT=\$$?" >> $@
	@echo "  if [ 0 != \$${RESULT} ]; then" >> $@
	@echo "    \$${ECHO} \"FAILED(\$${RESULT}) \$${ERROR}\"" >> $@
	@echo "    exit 1" >> $@
	@echo "  else" >> $@
	@echo "    \$${ECHO} \"OK \$${ERROR}\"" >> $@
	@echo "  fi" >> $@
	@echo "  \$${ECHO} >> \$${FILE}" >> $@
	@echo "  NRUN=\$$((NRUN+1))" >> $@
	@echo "done" >> $@
	@echo >> $@
	@chmod +x $@

$(SPLDIR)/nek/rstr-perf.sh: $(SPLDIR)/nek/.make $(ROOTDIR)/Makefile
	@echo "#!/bin/sh" > $@
	@echo >> $@
	@echo "HERE=\$$(cd \$$(dirname \$$0); pwd -P)" >> $@
	@echo "ECHO=\$$(which echo)" >> $@
	@echo "FILE=\$${HERE}/rstr-perf.txt" >> $@
ifneq (,$(strip $(INDICES)))
	@echo "RUNS=\"$(INDICES)\"" >> $@
	@echo "RUNT=\"$(INDICES)\"" >> $@
else
	@echo "RUNS=\"4_4_4 8_8_8\"" >> $@
	@echo "RUNT=\"7_7_7 10_10_10\"" >> $@
endif
	@echo >> $@
	@echo "if [ \"\" != \"\$$1\" ]; then" >> $@
	@echo "  FILE=\$$1" >> $@
	@echo "  shift" >> $@
	@echo "fi" >> $@
	@echo "cat /dev/null > \$${FILE}" >> $@
	@echo >> $@
	@echo "NRUN=1" >> $@
	@echo "NRUNS=\$$(\$${ECHO} \$${RUNS} | wc -w)" >> $@
	@echo "NRUNT=\$$(\$${ECHO} \$${RUNT} | wc -w)" >> $@
	@echo "NMAX=\$$((NRUNS*NRUNT))" >> $@
	@echo "for RUN1 in \$${RUNS} ; do" >> $@
	@echo "  for RUN2 in \$${RUNT} ; do" >> $@
	@echo "  MVALUE=\$$(\$${ECHO} \$${RUN1} | cut -d_ -f1)" >> $@
	@echo "  NVALUE=\$$(\$${ECHO} \$${RUN1} | cut -d_ -f2)" >> $@
	@echo "  KVALUE=\$$(\$${ECHO} \$${RUN1} | cut -d_ -f3)" >> $@
	@echo "  MMVALUE=\$$(\$${ECHO} \$${RUN2} | cut -d_ -f1)" >> $@
	@echo "  NNVALUE=\$$(\$${ECHO} \$${RUN2} | cut -d_ -f2)" >> $@
	@echo "  KKVALUE=\$$(\$${ECHO} \$${RUN2} | cut -d_ -f3)" >> $@
	@echo "  >&2 \$\$${ECHO} -n \"\$${NRUN} of \$${NMAX} (M=\$${MVALUE} N=\$${NVALUE} K=\$${KVALUE})... \"" >> $@
	@echo "  ERROR=\$$({ CHECK=1 \$${HERE}/rstr.sh \$${MVALUE} \$${NVALUE} \$${KVALUE} \$${MMVALUE} \$${NNVALUE} \$${KKVALUE} \$$* >> \$${FILE}; } 2>&1)" >> $@
	@echo "  RESULT=\$$?" >> $@
	@echo "  if [ 0 != \$${RESULT} ]; then" >> $@
	@echo "    \$${ECHO} \"FAILED(\$${RESULT}) \$${ERROR}\"" >> $@
	@echo "    exit 1" >> $@
	@echo "  else" >> $@
	@echo "    \$${ECHO} \"OK \$${ERROR}\"" >> $@
	@echo "  fi" >> $@
	@echo "  \$${ECHO} >> \$${FILE}" >> $@
	@echo "  NRUN=\$$((NRUN+1))" >> $@
	@echo "done" >> $@
	@echo "done" >> $@
	@echo >> $@
	@chmod +x $@

.PHONY: test
test: test-cp2k

.PHONY: perf
perf: perf-cp2k

.PHONY: test-all
test-all: tests test-cp2k test-smm test-nek test-wrap

.PHONY: build-tests
build-tests: lib_hst
	@cd $(TSTDIR) && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS)

.PHONY: tests
tests: build-tests
	@cd $(TSTDIR) && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS) test

.PHONY: test-cp2k
test-cp2k: $(SPLDIR)/cp2k/cp2k-test.txt
$(SPLDIR)/cp2k/cp2k-test.txt: $(SPLDIR)/cp2k/cp2k-perf.sh lib_hst
	$(info ========================)
	$(info Running CP2K Code Sample)
	$(info ========================)
	@cd $(SPLDIR)/cp2k && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS) cp2k
	@$(SPLDIR)/cp2k/cp2k-perf.sh $@ $(shell echo $$(($(TESTSIZE) * 128)))

.PHONY: perf-cp2k
perf-cp2k: $(SPLDIR)/cp2k/cp2k-perf.txt
$(SPLDIR)/cp2k/cp2k-perf.txt: $(SPLDIR)/cp2k/cp2k-perf.sh lib_hst
	@cd $(SPLDIR)/cp2k && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS) cp2k
	@$(SPLDIR)/cp2k/cp2k-perf.sh $@

.PHONY: test-dgemm
test-dgemm: dgemm
	@cd $(SPLDIR)/dgemm && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=0 \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS) test

.PHONY: test-wrap
test-wrap: test-dgemm

.PHONY: test-smm
test-smm: $(SPLDIR)/smm/smm-test.txt
$(SPLDIR)/smm/smm-test.txt: $(SPLDIR)/smm/smmf-perf.sh lib_hst
	$(info =======================)
	$(info Running SMM Code Sample)
	$(info =======================)
	@cd $(SPLDIR)/smm && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS) smm
	@$(SPLDIR)/smm/smmf-perf.sh $@ $(shell echo $$(($(TESTSIZE) * -128)))

.PHONY: perf-smm
perf-smm: $(SPLDIR)/smm/smmf-perf.txt
$(SPLDIR)/smm/smmf-perf.txt: $(SPLDIR)/smm/smmf-perf.sh lib_hst
	@cd $(SPLDIR)/smm && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS) smm
	@$(SPLDIR)/smm/smmf-perf.sh $@

.PHONY: test-nek
test-nek: $(SPLDIR)/nek/axhm-perf.txt $(SPLDIR)/nek/grad-perf.txt $(SPLDIR)/nek/rstr-perf.txt
$(SPLDIR)/nek/axhm-perf.txt: $(SPLDIR)/nek/axhm-perf.sh lib_hst
	$(info =======================)
	$(info Running NEK/AXHM Sample)
	$(info =======================)
	@cd $(SPLDIR)/nek && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS) axhm
	@$(SPLDIR)/nek/axhm-perf.sh $@ $(shell echo $$(($(TESTSIZE) * -128)))
$(SPLDIR)/nek/grad-perf.txt: $(SPLDIR)/nek/grad-perf.sh lib_hst
	$(info =======================)
	$(info Running NEK/GRAD Sample)
	$(info =======================)
	@cd $(SPLDIR)/nek && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS) grad
	@$(SPLDIR)/nek/grad-perf.sh $@ $(shell echo $$(($(TESTSIZE) * -128)))
$(SPLDIR)/nek/rstr-perf.txt: $(SPLDIR)/nek/rstr-perf.sh lib_hst
	$(info =======================)
	$(info Running NEK/RSTR Sample)
	$(info =======================)
	@cd $(SPLDIR)/nek && $(MAKE) --no-print-directory \
		DEPSTATIC=$(STATIC) SYM=$(SYM) DBG=$(DBG) IPO=$(IPO) SSE=$(SSE) AVX=$(AVX) OFFLOAD=$(OFFLOAD) TRACE=$(TRACE) \
		EFLAGS=$(EFLAGS) ELDFLAGS=$(ELDFLAGS) ECXXFLAGS=$(ECXXFLAGS) ECFLAGS=$(ECFLAGS) EFCFLAGS=$(EFCFLAGS) rstr
	@$(SPLDIR)/nek/rstr-perf.sh $@ $(shell echo $$(($(TESTSIZE) * -128)))

$(DOCDIR)/libxsmm.pdf: $(DOCDIR)/.make $(ROOTDIR)/README.md
	$(eval TMPFILE = $(shell mktemp fileXXXXXX))
	@mv $(TMPFILE) $(TMPFILE).tex
	@pandoc -D latex | sed \
		-e 's/\(\\documentclass\[..*\]{..*}\)/\1\n\\pagenumbering{gobble}\n\\RedeclareSectionCommands[beforeskip=-1pt,afterskip=1pt]{subsection,subsubsection}/' \
		-e 's/\\usepackage{listings}/\\usepackage{listings}\\lstset{basicstyle=\\footnotesize\\ttfamily}/' > \
		$(TMPFILE).tex
	@sed \
		-e 's/https:\/\/raw\.githubusercontent\.com\/hfp\/libxsmm\/master\///' \
		-e 's/\[!\[..*\](https:\/\/travis-ci.org\/hfp\/libxsmm.svg?branch=..*)\](..*)//' \
		-e 's/\[\[..*\](..*)\]//' -e '/!\[..*\](..*)/{n;d}' \
		-e 's/<sub>/~/g' -e 's/<\/sub>/~/g' \
		-e 's/<sup>/^/g' -e 's/<\/sup>/^/g' \
		$(ROOTDIR)/README.md | \
	pandoc \
		--latex-engine=xelatex --template=$(TMPFILE).tex --listings \
		-f markdown_github+implicit_figures+all_symbols_escapable+subscript+superscript \
		-V documentclass=scrartcl \
		-V title-meta="LIBXSMM Documentation" \
		-V author-meta="Hans Pabst, Alexander Heinecke" \
		-V classoption=DIV=45 \
		-V linkcolor=black \
		-V citecolor=black \
		-V urlcolor=black \
		-o $@
	@rm $(TMPFILE).tex

$(DOCDIR)/cp2k.pdf: $(DOCDIR)/.make $(ROOTDIR)/documentation/cp2k.md
	$(eval TMPFILE = $(shell mktemp fileXXXXXX))
	@mv $(TMPFILE) $(TMPFILE).tex
	@pandoc -D latex | sed \
		-e 's/\(\\documentclass\[..*\]{..*}\)/\1\n\\pagenumbering{gobble}\n\\RedeclareSectionCommands[beforeskip=-1pt,afterskip=1pt]{subsection,subsubsection}/' \
		-e 's/\\usepackage{listings}/\\usepackage{listings}\\lstset{basicstyle=\\footnotesize\\ttfamily}/' > \
		$(TMPFILE).tex
	@sed \
		-e 's/https:\/\/raw\.githubusercontent\.com\/hfp\/libxsmm\/master\///' \
		-e 's/\[!\[..*\](https:\/\/travis-ci.org\/hfp\/libxsmm.svg?branch=..*)\](..*)//' \
		-e 's/\[\[..*\](..*)\]//' -e '/!\[..*\](..*)/{n;d}' \
		-e 's/<sub>/~/g' -e 's/<\/sub>/~/g' \
		-e 's/<sup>/^/g' -e 's/<\/sup>/^/g' \
		$(ROOTDIR)/documentation/cp2k.md | \
	pandoc \
		--latex-engine=xelatex --template=$(TMPFILE).tex --listings \
		-f markdown_github+implicit_figures+all_symbols_escapable+subscript+superscript \
		-V documentclass=scrartcl \
		-V title-meta="CP2K with LIBXSMM" \
		-V author-meta="Hans Pabst" \
		-V classoption=DIV=45 \
		-V linkcolor=black \
		-V citecolor=black \
		-V urlcolor=black \
		-o $@
	@rm $(TMPFILE).tex

.PHONY: documentation
documentation: $(DOCDIR)/libxsmm.pdf $(DOCDIR)/cp2k.pdf

.PHONY: clean-minimal
clean-minimal:
	@rm -f $(SCRDIR)/libxsmm_utilities.pyc
	@rm -rf $(SCRDIR)/__pycache__
	@touch $(SPLDIR)/cp2k/.make
	@touch $(SPLDIR)/smm/.make
	@touch $(SPLDIR)/nek/.make
	@touch $(INCDIR)/.make

.PHONY: clean
clean: clean-minimal
	@rm -f $(OBJECTS) $(FTNOBJS) $(SRCFILES_KERNELS)
	@rm -f $(BLDDIR)/libxsmm_dispatch.h

.PHONY: realclean
realclean: clean
ifneq ($(abspath $(BLDDIR)),$(ROOTDIR))
ifneq ($(abspath $(BLDDIR)),$(abspath .))
	@rm -rf $(BLDDIR)
endif
endif
ifneq ($(abspath $(OUTDIR)),$(ROOTDIR))
ifneq ($(abspath $(OUTDIR)),$(abspath .))
	@rm -rf $(OUTDIR)
endif
endif
ifneq ($(abspath $(BINDIR)),$(ROOTDIR))
ifneq ($(abspath $(BINDIR)),$(abspath .))
	@rm -rf $(BINDIR)
endif
endif
ifneq (,$(wildcard $(OUTDIR)))
	@rm -f $(OUTDIR)/libxsmm.$(LIBEXT) $(OUTDIR)/mic/libxsmm.$(LIBEXT)
	@rm -f $(OUTDIR)/libxsmmf.$(LIBEXT) $(OUTDIR)/mic/libxsmmf.$(LIBEXT)
	@rm -f $(OUTDIR)/libxsmmext.$(LIBEXT) $(OUTDIR)/mic/libxsmmext.$(LIBEXT)
	@rm -f $(OUTDIR)/libxsmmgen.$(LIBEXT)
endif
ifneq (,$(wildcard $(BINDIR)))
	@rm -f $(BINDIR)/libxsmm_gemm_generator
endif
	@rm -f *.gcno *.gcda *.gcov
	@rm -f $(SPLDIR)/cp2k/cp2k-perf.sh
	@rm -f $(SPLDIR)/smm/smmf-perf.sh
	@rm -f $(SPLDIR)/nek/grad-perf.sh
	@rm -f $(SPLDIR)/nek/axhm-perf.sh
	@rm -f $(SPLDIR)/nek/rstr-perf.sh
	@rm -f $(INCDIR)/libxsmm.modmic
	@rm -f $(INCDIR)/libxsmm.mod
	@rm -f $(INCDIR)/libxsmm.f
	@rm -f $(INCDIR)/libxsmm.h
	@rm -f $(INCDIR)/.make
	@rm -f $(DOCDIR)/.make
	@rm -f .make .state

.PHONY: clean-all
clean-all: clean
	@cd $(TSTDIR)           && $(MAKE) --no-print-directory clean-minimal
	@cd $(SPLDIR)/cp2k      && $(MAKE) --no-print-directory clean-minimal
	@cd $(SPLDIR)/dgemm     && $(MAKE) --no-print-directory clean-minimal
	@cd $(SPLDIR)/dispatch  && $(MAKE) --no-print-directory clean-minimal
	@cd $(SPLDIR)/nek       && $(MAKE) --no-print-directory clean-minimal
	@cd $(SPLDIR)/smm       && $(MAKE) --no-print-directory clean-minimal

.PHONY: realclean-all
realclean-all: realclean
	@cd $(TSTDIR)           && $(MAKE) --no-print-directory realclean
	@cd $(SPLDIR)/cp2k      && $(MAKE) --no-print-directory realclean
	@cd $(SPLDIR)/dgemm     && $(MAKE) --no-print-directory realclean
	@cd $(SPLDIR)/dispatch  && $(MAKE) --no-print-directory realclean
	@cd $(SPLDIR)/nek       && $(MAKE) --no-print-directory realclean
	@cd $(SPLDIR)/smm       && $(MAKE) --no-print-directory realclean

# Dummy prefix
ifneq (,$(strip $(PREFIX)))
INSTALL_ROOT = $(PREFIX)
else
INSTALL_ROOT = .
endif

.PHONY: install-minimal
install-minimal: lib generator
ifneq ($(abspath $(INSTALL_ROOT)),$(abspath .))
	@echo
	@echo "LIBXSMM installing binaries..."
	@mkdir -p $(INSTALL_ROOT)/$(POUTDIR) $(INSTALL_ROOT)/$(PBINDIR) $(INSTALL_ROOT)/$(PINCDIR)
	@cp -v $(OUTDIR)/libxsmmgen.$(DLIBEXT) $(INSTALL_ROOT)/$(POUTDIR) 2> /dev/null || true
	@cp -v $(OUTDIR)/libxsmmgen.$(SLIBEXT) $(INSTALL_ROOT)/$(POUTDIR) 2> /dev/null || true
	@cp -v $(OUTDIR)/libxsmmext.$(DLIBEXT) $(INSTALL_ROOT)/$(POUTDIR) 2> /dev/null || true
	@cp -v $(OUTDIR)/libxsmmf.$(DLIBEXT) $(INSTALL_ROOT)/$(POUTDIR) 2> /dev/null || true
	@cp -v $(OUTDIR)/libxsmmf.$(SLIBEXT) $(INSTALL_ROOT)/$(POUTDIR) 2> /dev/null || true
	@cp -v $(OUTDIR)/libxsmm.$(DLIBEXT) $(INSTALL_ROOT)/$(POUTDIR) 2> /dev/null || true
	@cp -v $(OUTDIR)/libxsmm.$(SLIBEXT) $(INSTALL_ROOT)/$(POUTDIR) 2> /dev/null || true
	@if [ -e $(OUTDIR)/mic/libxsmmext.$(DLIBEXT) ]; then \
		mkdir -p $(INSTALL_ROOT)/$(POUTDIR)/mic ; \
		cp -v $(OUTDIR)/mic/libxsmmext.$(DLIBEXT) $(INSTALL_ROOT)/$(POUTDIR)/mic ; \
	fi
	@if [ -e $(OUTDIR)/mic/libxsmmf.$(DLIBEXT) ]; then \
		mkdir -p $(INSTALL_ROOT)/$(POUTDIR)/mic ; \
		cp -v $(OUTDIR)/mic/libxsmmf.$(DLIBEXT) $(INSTALL_ROOT)/$(POUTDIR)/mic ; \
	fi
	@if [ -e $(OUTDIR)/mic/libxsmmf.$(SLIBEXT) ]; then \
		mkdir -p $(INSTALL_ROOT)/$(POUTDIR)/mic ; \
		cp -v $(OUTDIR)/mic/libxsmmf.$(SLIBEXT) $(INSTALL_ROOT)/$(POUTDIR)/mic ; \
	fi

	@if [ -e $(OUTDIR)/mic/libxsmm.$(DLIBEXT) ]; then \
		mkdir -p $(INSTALL_ROOT)/$(POUTDIR)/mic ; \
		cp -v $(OUTDIR)/mic/libxsmm.$(DLIBEXT) $(INSTALL_ROOT)/$(POUTDIR)/mic ; \
	fi
	@if [ -e $(OUTDIR)/mic/libxsmm.$(SLIBEXT) ]; then \
		mkdir -p $(INSTALL_ROOT)/$(POUTDIR)/mic ; \
		cp -v $(OUTDIR)/mic/libxsmm.$(SLIBEXT) $(INSTALL_ROOT)/$(POUTDIR)/mic ; \
	fi
	@cp -v $(BINDIR)/libxsmm_gemm_generator $(INSTALL_ROOT)/$(PBINDIR) 2> /dev/null || true
	@cp -v $(INCDIR)/*.mod* $(INSTALL_ROOT)/$(PINCDIR) 2> /dev/null || true
	@cp -v $(INCDIR)/libxsmm*.h $(INSTALL_ROOT)/$(PINCDIR)
	@cp -v $(INCDIR)/libxsmm.f $(INSTALL_ROOT)/$(PINCDIR)
endif

.PHONY: install
install: install-minimal
ifneq ($(abspath $(INSTALL_ROOT)),$(abspath .))
	@echo
	@echo "LIBXSMM installing documentation..."
	@mkdir -p $(INSTALL_ROOT)/$(PDOCDIR)
	@cp -v $(ROOTDIR)/$(DOCDIR)/*.pdf $(INSTALL_ROOT)/$(PDOCDIR)
	@cp -v $(ROOTDIR)/$(DOCDIR)/*.md $(INSTALL_ROOT)/$(PDOCDIR)
	@cp -v $(ROOTDIR)/version.txt $(INSTALL_ROOT)/$(PDOCDIR)
	@cp -v $(ROOTDIR)/README.md $(INSTALL_ROOT)/$(PDOCDIR)
	@cp -v $(ROOTDIR)/LICENSE $(INSTALL_ROOT)/$(PDOCDIR)
endif

.PHONY: install-all
install-all: install samples
ifneq ($(abspath $(INSTALL_ROOT)),$(abspath .))
	@echo
	@echo "LIBXSMM installing samples..."
	@cp -v $(addprefix $(SPLDIR)/cp2k/,cp2k cp2k.sh cp2k-perf* cp2k-plot.sh) $(INSTALL_ROOT)/$(PBINDIR) 2> /dev/null || true
	@cp -v $(addprefix $(SPLDIR)/dgemm/,dgemm-blas dgemm-blas.sh dgemm-wrap dgemm-wrap.sh dgemm-test.sh) $(INSTALL_ROOT)/$(PBINDIR) 2> /dev/null || true
	@cp -v $(addprefix $(SPLDIR)/dispatch/,dispatch dispatch.sh) $(INSTALL_ROOT)/$(PBINDIR) 2> /dev/null || true
	@cp -v $(addprefix $(SPLDIR)/nek/,axhm grad rstr *.sh) $(INSTALL_ROOT)/$(PBINDIR) 2> /dev/null || true
	@cp -v $(addprefix $(SPLDIR)/smm/,smm smm.sh smm-perf* smmf-perf.sh smm-plot.sh) $(INSTALL_ROOT)/$(PBINDIR) 2> /dev/null || true
	@cp -v $(addprefix $(SPLDIR)/smm/,specialized specialized.sh) $(INSTALL_ROOT)/$(PBINDIR) 2> /dev/null || true
	@cp -v $(addprefix $(SPLDIR)/smm/,dispatched dispatched.sh) $(INSTALL_ROOT)/$(PBINDIR) 2> /dev/null || true
	@cp -v $(addprefix $(SPLDIR)/smm/,inlined inlined.sh) $(INSTALL_ROOT)/$(PBINDIR) 2> /dev/null || true
	@cp -v $(addprefix $(SPLDIR)/smm/,blas blas.sh) $(INSTALL_ROOT)/$(PBINDIR) 2> /dev/null || true
endif

.PHONY: install-dev
install-dev: install-all build-tests
ifneq ($(abspath $(INSTALL_ROOT)),$(abspath .))
	@echo
	@echo "LIBXSMM installing tests..."
	@mkdir -p $(INSTALL_ROOT)/$(PTSTDIR)
	@cp -v $(basename $(shell ls -1 ${TSTDIR}/*.c 2> /dev/null | tr "\n" " ")) $(INSTALL_ROOT)/$(PTSTDIR) 2> /dev/null || true
endif

.PHONY: install-artifacts
install-artifacts: install-dev
ifneq ($(abspath $(INSTALL_ROOT)),$(abspath .))
	@echo
	@echo "LIBXSMM installing artifacts..."
	@mkdir -p $(INSTALL_ROOT)/$(PDOCDIR)/artifacts
	@cp -v .state $(INSTALL_ROOT)/$(PDOCDIR)/artifacts/make.txt
endif

