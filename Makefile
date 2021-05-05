SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

WORKDIR=target
DEVENV=external/mikanos-build/devenv
TARGET=target/mikanos.img

all: $(TARGET) $(WORKDIR)/OVMF_CODE.fd $(WORKDIR)/OVMF_VARS.fd
.PHONY: all

clean:
	$(RM) -r target
.PHONY: clean

FORCE:
.PHONY: FORCE

.PRECIOUS: $(WORKDIR)/%.img $(WORKDIR)/%.efi $(WORKDIR)/%.fd

$(WORKDIR)/kernel.elf: FORCE
	$(MAKE) -C kernel

apps:
	$(MAKE) -C apps
.PHONY: apps

run-qemu-%: $(WORKDIR)/%.img $(WORKDIR)/OVMF_CODE.fd $(WORKDIR)/OVMF_VARS.fd
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

$(WORKDIR)/%.fd: $(DEVENV)/%.fd Makefile
	mkdir -p $(@D)
	cp $< $@

ANOTHER_FILE=
APPS_DIR=
$(WORKDIR)/%.img: ./target/%.efi apps Makefile
	mkdir -p $(@D)
	qemu-img create -f raw $@ 200M
	mkfs.fat -n 'MIKAN OS' -s 2 -f 2 -R 32 -F 32 $@
	mkdir -p $(WORKDIR)/mnt
	sudo mount -o loop $@ $(WORKDIR)/mnt
	sudo mkdir -p $(WORKDIR)/mnt/EFI/BOOT/
	sudo cp $< $(WORKDIR)/mnt/EFI/BOOT/BOOTX64.EFI
	if [ "$(ANOTHER_FILE)" != "" ]; then
	    sudo cp "$(ANOTHER_FILE)" $(WORKDIR)/mnt/
	fi
	if [ "$(APPS_DIR)" != "" ]; then
	    sudo mkdir -p $(WORKDIR)/mnt/$(APPS_DIR)
	fi
	sudo cp $(WORKDIR)/apps/* $(WORKDIR)/mnt/$(APPS_DIR)/
	sudo umount $(WORKDIR)/mnt
$(WORKDIR)/mikanos.img: $(WORKDIR)/kernel.elf
$(WORKDIR)/mikanos.img: ANOTHER_FILE=$(WORKDIR)/kernel.elf
$(WORKDIR)/mikanos.img: APPS_DIR=apps

$(WORKDIR)/mikanos.efi: FORCE
	mkdir -p $(@D)
	(
	    cd external/edk2
	    set +eu
	    . ./edksetup.sh
	    set -eu
	    build
	)
	NEW_EFI=./external/edk2/Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi
	OLD_EFI=$@
	if ! [ -e $${OLD_EFI} ] || ! diff $${OLD_EFI} $${NEW_EFI} >/dev/null; then
	    cp $${NEW_EFI} $@
	fi

-include $(DEPS)
