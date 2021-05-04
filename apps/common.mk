ifndef REPOROOT
    $(error $$(REPOROOT) is not defined)
endif
ifndef WORKDIR
    $(error $$(WORKDIR) is not defined)
endif

.PRECIOUS: $(WORKDIR)/%.o

COMMON_OBJS=$(REPOROOT)/target/work/apps/syscall.o $(REPOROOT)/target/work/apps/newlib_support.o

DEVENV=$(REPOROOT)/external/mikanos-build/devenv
CPPFLAGS=\
    -I. \
    -I$(DEVENV)/x86_64-elf/include/c++/v1 \
    -I$(DEVENV)/x86_64-elf/include \
    -I$(DEVENV)/x86_64-elf/include \
    -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS
CFLAGS=\
    -O2 -Wall -g --target=x86_64-elf -ffreestanding -mcmodel=large \
    --std=c17
CXXFLAGS=\
    -O2 -Wall -g --target=x86_64-elf -ffreestanding -mcmodel=large \
    -fno-exceptions -fno-rtti -std=c++17
LDFLAGS=\
    -L$(DEVENV)/x86_64-elf/lib \
    --entry main -z norelro --image-base 0xffff800000000000 --static

$(WORKDIR)/%.o: %.cpp Makefile
	mkdir -p $(@D)
	clang++ -MMD $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(WORKDIR)/%.o: %.c Makefile
	mkdir -p $(@D)
	clang -MMD $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(WORKDIR)/%.o: %.asm Makefile
	mkdir -p $(@D)
	nasm -f elf64 -o $@ $<

-include $(DEPS)
