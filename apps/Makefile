SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

SUBDIRS=$(shell find . -mindepth 1 -maxdepth 1 -type d -printf '%P\n')

REPOROOT=..
WORKDIR=$(REPOROOT)/target/work/apps

CPP_SRCS=$(wildcard *.cpp)
C_SRCS=$(wildcard *.c)
ASM_SRCS=$(wildcard *.asm)
OBJS=$(addprefix $(WORKDIR)/,$(CPP_SRCS:.cpp=.o) $(C_SRCS:.c=.o) $(ASM_SRCS:.asm=.o))
DEPS=$(addprefix $(WORKDIR)/,$(CPP_SRCS:.cpp=.d) $(C_SRCS:.c=.d))

all:
.PHONY: all

clean:
.PHONY: clean

define define-rules
all-$(1): $(OBJS)
	$(MAKE) -C $(1) all
.PHONY: all-$(1)
all: all-$(1)

clean-$(1):
	$(MAKE) -C $(1) clean
.PHONY: clean-$(1)
clean: clean-$(1)
endef

$(foreach subdir,$(SUBDIRS),$(eval $(call define-rules,$(subdir))))

include common.mk
