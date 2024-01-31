<div align="right">
    English | <a href="./README-zh.md">中文</a>
</div>

<div align="center">

# uniform-minios

uniform-minios, namely unios, is a much more consistent impl of NWPU miniOS.

[Learn about unios](#learn-about-unios) •
[Installation](#installation) •
[Getting started](#getting-started) •
[Other](#contributors)

</div>

# Learn about unios

unios, full name union-miniOS, is derived from nwpu miniOS. unios aims to provide a more consistent implementation of nwpu miniOS in terms of structure, coding, etc.

# Installation

## Requirement

<details close>

  <summary><b>Minimum requirements</b></summary>

- GNU make >= 4.0
- GNU bash >= 5.0
- GNU coreutils >= 9.4
- GCC for x86 arch with 32 bit support >= 9.0
- NASM >= 2.0

</details>

<details open>

  <summary><b>Recommended environments</b></summary>

- GNU make >= 4.4
- GNU bash >= 5.1
- GNU coreutils >= 9.4
- GCC for x86 arch with 32 bit support >= 13.2
- NASM >= 2.16
- GNU gdb >= 13.2
- qemu-system-i386 >= 8.1
- python 3
- pip for python 3 >= 23.3
- Git >= 2.42
- clang-format >= 17.0
- bear >= 3.1

</details>

<details close>

  <summary><b>Recommended workspace configuration</b></summary>

- Visual Studio Code
- \[Extension] llvm-vs-code-extensions.vscode-clangd
- \[Extension] Gruntfuggly.todo-tree
- \[Extension] lextudio.restructuredtext
- \[Extension] ms-python.python
- \[Extension] usernamehw.errorlens
- \[Extension] alefragnani.Bookmarks
- \[Extension] kevinkyang.auto-comment-blocks
- \[Extension] xaver.clang-format

</details>

## Build virtual disk image

```bash
make image
```

unios is not currently supported for installation on physical devices, you can try to write unios to the storage media to be installed by following the image building method provided in `project/rules-image.mk`.

# Getting started

## Support commands

The following section gives an outline of the provided make commands, and more details can be found in the doc.

```plain
all:          same as build [DEFAULT]
build:        build all, include unios image, kernel debug file and tools, alias `b`
clean:        clean up the build dir
config:       collect envs and generate compile_commands.json, alias `conf`
debug:        run unios using qemu in debug mode, alias `b`
doc:          build doc and serve continuously
dup-cc:       generate compile_commands.json
format:       format sources, alias `fmt`
image:        build unios image
install:      install targets to root of build dir, alias `i`
kernel:       build unios kernel, alias `krnl`
lib:          build unios runtime library
monitor-real: same as monitor, but start up in real mode, alias `mon-real`
monitor:      run gdb and connect to remote provided by qemu, alias `mon`
pre-doc:      install deps for building the doc
run:          run unios using qemu, alias `r`
tools:        build tools
user:         build user programs and create archive
```

## Run in QEMU

```sh
make r
```

## Debug with QEMU and GDB

Run qemu in debug mode in one of your terminals, and another to run gdb for debug.

```sh
make d
```

```sh
make mon
```

`monitor` do not rebuild kernel debug file, so make sure to run `debug` first before running the gdb.

# Contributors

See [Contributing](https://github.com/zymelaii/uniform-minios/graphs/contributors) for details. Thanks to all the people who already contributed!

![Contributors](https://contributors-img.web.app/image?repo=zymelaii/uniform-minios&max=500)
