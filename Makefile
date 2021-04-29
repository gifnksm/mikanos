SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

DEVENV=./external/mikanos-build/devenv

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
	    -drive if=ide,index=0,media=disk,format=raw,file=$<
.PHONY: run-qemu-%

./target/OVMF_CODE.fd: $(DEVENV)/OVMF_CODE.fd update-submodule
	cp $< $@

./target/OVMF_VARS.fd: $(DEVENV)/OVMF_VARS.fd update-submodule
	cp $< $@

./target/%.img: ./target/%.efi | ./target/
	qemu-img create -f raw $@ 200M
	mkfs.fat -n 'MIKAN OS' -s 2 -f 2 -R 32 -F 32 $@
	mkdir -p ./target/mnt
	sudo mount -o loop $@ ./target/mnt
	sudo mkdir -p ./target/mnt/EFI/BOOT/
	sudo cp $< ./target/mnt/EFI/BOOT/BOOTX64.EFI
	sudo umount ./target/mnt

./target/mikanos.efi:
	cd external/edk2
	set +eu
	. ./edksetup.sh
	set -eu
	build
	cp ./Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi ../../$@
.PHONY: ./target/mikanos.efi
