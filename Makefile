SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

DEVENV=./external/mikanos-build/devenv
KERNEL_CPPFLAGS=\
    -I$(DEVENV)/x86_64-elf/include/c++/v1 \
    -I$(DEVENV)/x86_64-elf/include \
    -I$(DEVENV)/x86_64-elf/include \
    -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS

all: target/mikanos.img
.PHONY: all

clean:
	$(RM) -r target
.PHONY: clean

update-submodule:
	git submodule update --init --recursive
.PHONY: update-submodule

.PRECIOUS: ./target/%.img ./target/%.o

%/:
	mkdir -p $@

run-qemu-%: ./target/%.img ./target/OVMF_CODE.fd ./target/OVMF_VARS.fd update-submodule
		qemu-system-x86_64 \
	    -drive if=pflash,format=raw,readonly,file=./target/OVMF_CODE.fd \
	    -drive if=pflash,format=raw,file=./target/OVMF_VARS.fd \
	    -drive if=ide,index=0,media=disk,format=raw,file=$< \
	    -monitor stdio
.PHONY: run-qemu-%

./target/OVMF_CODE.fd: $(DEVENV)/OVMF_CODE.fd update-submodule
	cp $< $@

./target/OVMF_VARS.fd: $(DEVENV)/OVMF_VARS.fd update-submodule
	cp $< $@

ANOTHER_FILE=
./target/%.img: ./target/%.efi | ./target/
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

./target/mikanos.efi: | ./target/
	cd external/edk2
	set +eu
	. ./edksetup.sh
	set -eu
	build
	cp ./Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi ../../$@
.PHONY: ./target/mikanos.efi

./target/main.o: ./kernel/main.cpp | ./target/
	clang++ $(KERNEL_CPPFLAGS) -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
	    -fno-exceptions -fno-rtti --std=c++17 -c $< -o $@
./target/kernel.elf: ./target/main.o
	ld.lld --entry KernelMain -z norelro --image-base 0x100000 --static -nmagic \
	    -o $@ $<
