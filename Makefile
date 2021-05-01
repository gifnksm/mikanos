SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

CPP_SRCS=$(shell find ./kernel/ -type f -name '*.cpp')
C_SRCS=$(shell find ./kernel/ -type f -name '*.c')
ASM_SRCS=$(shell find ./kernel/ -type f -name '*.asm')
OBJS=$(patsubst ./%.cpp,./target/%.o,$(CPP_SRCS)) $(patsubst ./%.c,./target/%.o,$(C_SRCS)) $(patsubst ./%.asm,./target/%.o,$(ASM_SRCS))
DEPS=$(patsubst ./%.cpp,./target/%.d,$(CPP_SRCS)) $(patsubst ./%.c,./target/%.d,$(C_SRCS))

OBJS+=./target/kernel/hankaku.o

DEVENV=./external/mikanos-build/devenv
CPPFLAGS=\
    -Ikernel \
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

all: target/mikanos.img
.PHONY: all

clean:
	$(RM) -r target
.PHONY: clean

update-submodule:
	git submodule update --init --recursive
.PHONY: update-submodule

.PRECIOUS: ./target/%.img ./target/%.o ./target/%.fd

%/:
	mkdir -p $@

run-qemu-%: ./target/%.img ./target/OVMF_CODE.fd ./target/OVMF_VARS.fd Makefile update-submodule
	qemu-system-x86_64 \
	    -m 1G \
	    -drive if=pflash,format=raw,readonly,file=./target/OVMF_CODE.fd \
	    -drive if=pflash,format=raw,file=./target/OVMF_VARS.fd \
	    -drive if=ide,index=0,media=disk,format=raw,file=$< \
	    -device nec-usb-xhci,id=xhci \
	    -device usb-mouse \
	    -device usb-kbd \
	    -monitor stdio
.PHONY: run-qemu-%

./target/%.fd: $(DEVENV)/%.fd Makefile update-submodule
	cp $< $@

ANOTHER_FILE=
./target/%.img: ./target/%.efi Makefile  | ./target/
	qemu-img create -f raw $@ 200M
	mkfs.fat -n 'MIKAN OS' -s 2 -f 2 -R 32 -F 32 $@
	mkdir -p ./target/mnt
	sudo mount -o loop $@ ./target/mnt
	sudo mkdir -p ./target/mnt/EFI/BOOT/
	sudo cp $< ./target/mnt/EFI/BOOT/BOOTX64.EFI
	if [ "${ANOTHER_FILE}" != "" ]; then
	    sudo cp "${ANOTHER_FILE}" ./target/mnt/
	fi
	sudo umount ./target/mnt
./target/mikanos.img: ./target/kernel.elf
./target/mikanos.img: ANOTHER_FILE=./target/kernel.elf

./target/mikanos.efi: Makefile | ./target/
	cd external/edk2
	set +eu
	. ./edksetup.sh
	set -eu
	build
	cp ./Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi ../../$@
.PHONY: ./target/mikanos.efi

./target/%.o: ./%.cpp Makefile
	mkdir -p $(@D)
	clang++ -MMD $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
./target/%.o: ./%.c Makefile | ./target/
	clang -MMD $(CPPFLAGS) $(CFLAGS) -c $< -o $@
./target/%.o: ./%.asm Makefile | ./target/
	nasm -f elf64 -o $@ $<
./target/kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o $@ $(OBJS) -lc -lc++ -lc++abi

./target/kernel/hankaku.bin: ./kernel/hankaku.txt Makefile
	mkdir -p $(@D)
	./tools/makefont -o $@ $<
./target/kernel/hankaku.o: ./target/kernel/hankaku.bin
	OUTPUT=$$(readlink -f $@)
	cd $(<D)
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $(<F) $${OUTPUT}

-include $(DEPS)
