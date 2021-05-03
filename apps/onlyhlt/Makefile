SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

TARGET=../../target/apps/onlyhlt

all: $(TARGET)
.PHONY: all

clean:
	$(RM) -r $(TARGET)
.PHONY: clean

../../target/apps/%: %.asm Makefile
	mkdir -p $(@D)
	nasm -f bin -o $@ $<
