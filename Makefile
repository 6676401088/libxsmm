DIR_MIC := .
OBJDIR_MIC := build
INCDIR_MIC := $(DIR_MIC)
SRCDIR_MIC := src
LIBDIR_MIC := lib

INDICES_M := $(shell seq 1 8)
INDICES_N := $(shell seq 1 8)
INDICES_K := $(shell seq 1 8)
INDICES := $(shell bash -c 'for m in $(INDICES_M); do ( for n in $(INDICES_N); do ( for k in $(INDICES_K); do echo "$${m}_$${n}_$${k}"; done ); done ); done')

LIB_MIC := libmic_$(firstword $(INDICES))__$(lastword $(INDICES)).a
SRC_MIC := xsmm_knc.c
INC_MIC := xsmm_knc.h

TARGET_COMPILE_C_MIC := icc -offload-attribute-target=mic -mkl=sequential -std=c99
AR := xiar -qoffload-build

SRCFILES_MIC := $(patsubst %,dc_small_dnn_%.c,$(INDICES))
OBJFILES_MIC := $(patsubst %,$(OBJDIR_MIC)/dc_small_dnn_%.o,$(INDICES))

lib_mic: $(LIBDIR_MIC)/$(LIB_MIC)
$(LIBDIR_MIC)/$(LIB_MIC): $(OBJFILES_MIC)
	@mkdir -p $(LIBDIR_MIC)
	$(AR) -rs $@ $^

header_mic: $(INCDIR_MIC)/$(INC_MIC)
$(INCDIR_MIC)/$(INC_MIC):
	@mkdir -p $(INCDIR_MIC)
	@echo "#ifndef XSMM_KNC_H" > $@
	@echo "#define XSMM_KNC_H" >> $@
	@echo >> $@
	@echo >> $@
	#@python $(DIR_MIC)/xsmm_knc_geninc.py >> $@ # TODO
	@bash -c 'for i in $(INDICES); do ( python $(DIR_MIC)/xsmm_knc_geninc.py `echo $${i} | tr "_" " "` ); done' >> $@
	@echo >> $@
	@echo "#endif // XSMM_KNC_H" >> $@

main_mic: $(SRCDIR_MIC)/$(SRC_MIC)
$(SRCDIR_MIC)/$(SRC_MIC):
	@mkdir -p $(SRCDIR_MIC)
	#@python $(DIR_MIC)/xsmm_knc_genmain.py ? ? ? > $@ # TODO

source_mic: $(addprefix $(SRCDIR_MIC)/,$(SRCFILES_MIC))
$(SRCDIR_MIC)/%.c:
	@mkdir -p $(SRCDIR_MIC)
	@python $(DIR_MIC)/xsmm_knc_gensrc.py `echo $* | awk -F_ '{ print $$4" "$$5" "$$6 }'` > $@

compile_mic: $(OBJFILES_MIC)
$(OBJDIR_MIC)/%.o: $(SRCDIR_MIC)/%.c header_mic #main_mic
	@mkdir -p $(OBJDIR_MIC)
	${TARGET_COMPILE_C_MIC} -I$(DIR_MIC) -c $< -o $@

all: $(LIBDIR_MIC)/$(LIB_MIC)

clean:
	rm -rf $(SRCDIR_MIC) $(OBJDIR_MIC) $(LIBDIR_MIC) $(INCDIR_MIC)/$(INC_MIC) $(DIR_MIC)/*~
