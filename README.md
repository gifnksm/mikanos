# MikanOS

https://zero.osdev.jp/

Arch Linux on WSL2 + systemd で実装。

## 利用したソフトウェア一覧

* 第1章
  * `hexedit`
  * `qemu`
  * `dosfstools`
  * `clang`
  * `lld`
* 第2章
  * `llvm`
* 第6章
  * `nasm`

## 実行方法

### 初期設定 (初回のみ)

```console
$ git submodule update --init --recursive
$ ./scripts/setup_edk2
$ ./scripts/setup_stdlib
```

### QEMU で実行

カーネルや UEFI 等のビルドを行い、 QEMU で実行します。

```console
$ make run-qemu-mikanos
```

### ディスクイメージをビルド

```console
$ make target/mikanos.img
```

### カーネルをビルド

```console
$ make target/kernel.elf
```

### `compiler_commands.json` の更新

`compiler_commands.json` を更新する場合、以下のように `bear` を使うと良い。

```console
# クリーンビルドしたい場合
$ touch Makefile MikanLoaderPkg/* kernel/*
$ bear --append -- make target/mikanos.img
```
