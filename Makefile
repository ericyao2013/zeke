
# Zeke - Main Makefile
#
# Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
# Copyright (c) 2012, 2013, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met: 
#
#1. Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer. 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution. 
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Configuration files ##########################################################
# Kernel config
CONFIG_MK  = ./config/config.mk
# Autogenerated header file
AUTOCONF_H = ./config/autoconf.h
################################################################################
include $(CONFIG_MK)
ifndef configMCU_MODEL
$(error Missing configMCU_MODEL!)
endif
ifndef configARCH
$(error Missing configARCH!)
endif

# Makefiles for modules ########################################################
# Atm for sake of simplicity we keep all makefiles on single level
MODMKFILES = $(wildcard ./modmakefiles/*.mk)
include $(MODMKFILES)
# Following will generate a list of module names
ALLMODULES = $(basename $(notdir $(MODMKFILES)))
# Remove modules that do not contain any compilation units
MODULES += $(foreach var, $(ALLMODULES), $(if $(strip $($(var)-SRC-1) $($(var)-ASRC-1)), $(var),))

# Generic Compiler Options #####################################################
ARMGNU   = arm-none-eabi
CC       = clang
CCFLAGS  = -std=c99 -emit-llvm -Wall -ffreestanding -O2
CCFLAGS += -nostdlib -nostdinc -v
CCFLAGS += -m32
OPT      = opt-3.3
OFLAGS   = -std-compile-opts
LLC      = llc-3.3
LLCFLAGS = -mtriple=$(ARMGNU)
ASFLAGS  =#
LDFLAGS  =#
################################################################################

# Target Specific Compiler Options & Special files #############################
include ./target.mak
################################################################################
ifndef ASFLAGS
	$(error Missing ASFLAGS! Wrong configARCH?)
endif

# Check that MEMMAP and STARTUP are defined
ifndef MEMMAP
	$(error Missing MEMMAP! Wrong configMCU_MODEL?)
endif
ifndef STARTUP
	$(error Missing STARTUP! Wrong configMCU_MODEL?)
endif

# Check that CRT is defined
ifndef CRT
	$(error Missing CRT! Wrong configMCU_MODEL or configARCH?)
endif
CRT_DIR = $(dir $(CRT))

# Include Dirs #################################################################
IDIR   = ./include ./config ./src
AIDIR += ./config
################################################################################
IDIR  := $(patsubst %,-I%,$(subst :, ,$(IDIR)))
AIDIR := $(patsubst %,-I%,$(subst :, ,$(AIDIR)))

# Select & Include Modules #####################################################
# Available selections for source code files:
SRC-	=# C sources
SRC-0  	=#
SRC-1  	=#
ASRC-  	=# Assembly sources
ASRC-0 	=#
ASRC-1	=#
# (A)SRC- and (A)SRC-0 meaning that submodule won't be compiled

# Include sources from top modules
SRC-1 += $(foreach var,$(MODULES), $($(var)-SRC-1))
ASRC-1 += $(foreach var,$(MODULES), $($(var)-ASRC-1))
################################################################################

# Parse file names #############################################################
# Assembly Obj files
ASOBJS 	:= $(patsubst %.S, %.o, $(ASRC-1))

# C Obj files
BCS  	:= $(patsubst %.c, %.bc, $(SRC-1))
OBJS 	:= $(patsubst %.c, %.o, $(SRC-1))

STARTUP_O = $(patsubst %.S, %.o, $(STARTUP))

# A files for modules
MODAS = $(addsuffix .a, $(MODULES))
################################################################################

# We use suffixes because it's fun
.SUFFIXES:						# Delete the default suffixes
.SUFFIXES: .c .bc .o .h .S ._S	# Define our suffix list

# Targets ######################################################################
# Help lines
# '# target: '		Generic target
# '# target_comp'	Generic target
# '# target_test'	Testing target
# '# target_clean'	Cleaning target
# '# target_doc'	Documentation target

# target_comp: all - Make config and compile kernel
all: config kernel

# target_doc: doc - Compile all documentation.
doc: doc-doxy doc-book

# target_doc: doc-doxy - Compile doxygen documentation.
doc-doxy:
	doxygen

# target_doc: doc-book - Compile Zeke book.
doc-book:
	@echo TODO

# target_comp: config - Update configuration from $(CONFIG_MK)
config: $(AUTOCONF_H)

# target_test: test - Run all portable unit tests
test:
	-cd test/universal && make
	-cd test/kstring && make

# target_clean: clean-test - Clean portable test targets
clean-test:
	cd test/universal && make clean
	cd test/kstring && make clean

$(AUTOCONF_H): $(CONFIG_MK)
	./tools/aconf.sh $(CONFIG_MK) $(AUTOCONF_H)

$(CONFIG_MK):
# End of config

kernel: kernel.bin

$(STARTUP_O): $(STARTUP)
	@echo "AS $@"
	@$(ARMGNU)-as $< -o $(STARTUP_O)

$(ASOBJS): $(ASRC-1)
	@echo "AS $@"
	@unifdefall $(AIDIR) $*.S | $(ARMGNU)-as -am $(AIDIR) -o $@

$(BCS): $(SRC-1)
	@echo "CC $@"
	@$(CC) $(CCFLAGS) $(IDIR) -c $*.c -o $@

$(OBJS): $(BCS)
	$(eval CUR_OPT := $*.opt.bc)
	$(eval CUR_OPT_S := $*.opt.s)
	@echo "AS $@"
	@$(OPT) $(OFLAGS) $*.bc -o $(CUR_OPT)
	@$(LLC) $(LLCFLAGS) $(CUR_OPT) -o $(CUR_OPT_S)
	@$(ARMGNU)-as $(CUR_OPT_S) -o $@ $(ASFLAGS)

$(CRT):
	make -C $(CRT_DIR) all

$(MODAS): $(ASOBJS) $(OBJS)
	$(eval CUR_MODULE := $(basename $@))
	$(eval CUR_OBJS := $(patsubst %.c, %.o, $($(CUR_MODULE)-SRC-1)))
	$(eval CUR_OBJS += $(patsubst %.S, %.o, $($(CUR_MODULE)-ASRC-1)))
	@echo "AR $@"
	@ar rcs $@ $(CUR_OBJS)

kernel.bin: $(MEMMAP) $(STARTUP_O) $(MODAS) $(CRT)
	$(ARMGNU)-ld -o kernel.elf -T $(MEMMAP) $(LDFLAGS) $(STARTUP_O) --whole-archive $(MODAS) --no-whole-archive $(CRT)
	$(ARMGNU)-objdump -D kernel.elf > kernel.list
	$(ARMGNU)-objcopy kernel.elf -O binary kernel.bin

# target: help - Display callable targets.
help:
	./tools/help.sh

.PHONY: config kernel $(CRT) test clean

# target_clean: clean - Clean all targets
clean: clean-test
	rm -f $(AUTOCONF_H)
	rm -f $(ASOBJS) $(OBJS) $(STARTUP_O)
	rm -f $(BCS)
	find . -type f -name "*.bc" -exec rm -f {} \;
	find . -type f -name "*.opt.bc" -exec rm -f {} \;
	find . -type f -name "*.opt.s" -exec rm -f {} \;
	rm -f *.bin
	rm -f *.elf
	rm -f *.list
	rm -f *.a
	$(MAKE) -C $(CRT_DIR) clean
