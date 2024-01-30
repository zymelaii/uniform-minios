<div align="right">
    <a href="./README.md">English</a> | 中文
</div>

<div align="center">

# uniform-minios

uniform-minios，一个全方位高度一致的 NWPU miniOS 实现。

[关于 unios](#关于-unios) •
[安装说明](#安装说明) •
[使用 unios](#使用-unios) •
[其它](#贡献者)

</div>

# 关于 unios

unios，全名 uniform-minios，由 NWPU miniOS 派生而来，旨在提供一个在结构、编码等各方面更加一致的内核实现。

# 安装说明

## 环境要求

<details close>

  <summary><b>最低要求</b></summary>

- GNU make >= 4.0
- GNU bash >= 5.0
- GNU coreutils >= 9.4
- GCC for x86 arch with 32 bit support >= 9.0
- NASM >= 2.0

</details>

<details open>

  <summary><b>推荐环境配置</b></summary>

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
- clangd >= 15.0

</details>

<details close>

  <summary><b>推荐工作区配置</b></summary>

- Visual Studio Code
- \[插件] llvm-vs-code-extensions.vscode-clangd
- \[插件] Gruntfuggly.todo-tree
- \[插件] lextudio.restructuredtext
- \[插件] ms-python.python
- \[插件] usernamehw.errorlens
- \[插件] alefragnani.Bookmarks
- \[插件] kevinkyang.auto-comment-blocks

</details>

## 构建虚拟磁盘映像

```bash
make build
```

unios 当前不支持安装至实体设备，你可以尝试按照 `project/rules-image.mk` 中提供的映像构建方法将 unios 写入待安装的存储介质。

# 使用 unios

## 命令列表

该部分简单罗列了可用的 make 命令，关于更多细节请查阅文档。

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

## 在 QEMU 中运行

```sh
make r
```

## 使用 QEMU 和 GDB 调试

在其中一个终端以调试模式启动 QEMU，并在另一个终端启动 GDB。

```sh
make d
```

```sh
make mon
```

`monitor` 命令不会重新构建内核调试文件，故请保证在启动 GDB 之前首先运行 `debug` 命令。

# 贡献者

感谢所有对 unios 做出贡献的人员！🚪 [贡献详情](https://github.com/zymelaii/uniform-minios/graphs/contributors)

![贡献者列表](https://contributors-img.web.app/image?repo=zymelaii/uniform-minios&max=500)
