SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

WORKDIR=../../target/work/apps/rpn
TARGET=../../target/apps/rpn

.PRECIOUS: $(WORKDIR)/%.o

CPP_SRCS=$(shell find . -type f -name '*.cpp' -printf '%P\n')

OBJS=$(addprefix $(WORKDIR)/,$(CPP_SRCS:.cpp=.o))
DEPS=$(addprefix $(WORKDIR)/,$(CPP_SRCS:.cpp=.d))

DEVENV=../../external/mikanos-build/devenv
CPPFLAGS=\
    -I. \
    -I$(DEVENV)/x86_64-elf/include/c++/v1 \
    -I$(DEVENV)/x86_64-elf/include \
    -I$(DEVENV)/x86_64-elf/include \
    -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS
CXXFLAGS=\
    -O2 -Wall -g --target=x86_64-elf -ffreestanding -mcmodel=large \
    -fno-exceptions -fno-rtti -std=c++17
LDFLAGS=\
    -L$(DEVENV)/x86_64-elf/lib \
    --entry main -z norelro --image-base 0xffff800000000000 --static

all: $(TARGET)
.PHONY: all

clean:
	$(RM) -r $(TARGET) $(WORKDIR)
.PHONY: clean

$(TARGET): $(OBJS) Makefile
	mkdir -p $(@D)
	ld.lld $(LDFLAGS) -o $@ $(OBJS) -lc -lc++ -lc++abi

$(WORKDIR)/%.o: %.cpp Makefile
	mkdir -p $(@D)
	clang++ -MMD $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)
