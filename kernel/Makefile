SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

WORKDIR=../target/work/kernel
TARGET=../target/kernel.elf

CPP_SRCS=$(shell find . -type f -name '*.cpp' -printf '%P\n')
C_SRCS=$(shell find . -type f -name '*.c' -printf '%P\n')
ASM_SRCS=$(shell find . -type f -name '*.asm' -printf '%P\n')

OBJS=$(addprefix $(WORKDIR)/,$(CPP_SRCS:.cpp=.o) $(C_SRCS:.c=.o) $(ASM_SRCS:.asm=.o))
DEPS=$(addprefix $(WORKDIR)/,$(CPP_SRCS:.cpp=.d) $(C_SRCS:.c=.d))

OBJS+=$(WORKDIR)/hankaku.o

.PRECIOUS: $(WORKDIR)/%.o $(WORKDIR)/%.bin

DEVENV=../external/mikanos-build/devenv
CPPFLAGS=\
    -I. \
    -I$(DEVENV)/x86_64-elf/include/c++/v1 \
    -I$(DEVENV)/x86_64-elf/include \
    -I$(DEVENV)/x86_64-elf/include \
    -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS
CFLAGS=\
    -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
    --std=c17
CXXFLAGS=\
    -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
    -fno-exceptions -fno-rtti --std=c++17
LDFLAGS=\
    -L$(DEVENV)/x86_64-elf/lib \
    --entry KernelMain -z norelro --image-base 0x100000 --static

all: $(TARGET)
.PHONY: all

clean:
	$(RM) -r $(TARGET) $(WORKDIR)
.PHONY: clean

$(TARGET): $(OBJS) Makefile
	mkdir -p $(@D)
	ld.lld $(LDFLAGS) -o $@ $(OBJS) -lc -lc++ -lc++abi -lm

$(WORKDIR)/%.o: %.cpp Makefile
	mkdir -p $(@D)
	clang++ -MMD $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(WORKDIR)/%.o: %.c Makefile
	mkdir -p $(@D)
	clang -MMD $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(WORKDIR)/%.o: %.asm Makefile
	mkdir -p $(@D)
	nasm -f elf64 -o $@ $<
$(WORKDIR)/%.bin: %.txt Makefile
	mkdir -p $(@D)
	../tools/makefont -o $@ $<
$(WORKDIR)/%.o: $(WORKDIR)/%.bin
	OUTPUT=$$(readlink -f $@)
	cd $(<D)
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $(<F) $${OUTPUT}

-include $(DEPS)
