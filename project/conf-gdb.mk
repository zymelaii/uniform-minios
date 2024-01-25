# [in, out] GDB gdb executable
# [in] GDB_SCRIPTS_HOME path to user-defined gdb scripts
# [out] GDB_FLAGS options for gdb

GDB ?= gdb

GDB_SCRIPTS_HOME ?=

GDB_REALMODE_XML := $(GDB_SCRIPTS_HOME)target.xml

GDB_FLAGS := -q -nx
GDB_FLAGS += -x '$(GDB_SCRIPTS_HOME)connect-qemu.gdb'
GDB_FLAGS += -x '$(GDB_SCRIPTS_HOME)instr-level.gdb'
GDB_FLAGS += -ex 'set pagination off'
GDB_FLAGS += -ex 'set confirm off'
GDB_FLAGS += -ex 'set disassembly-flavor att'
GDB_FLAGS += -ex 'layout asm'
GDB_FLAGS += -ex 'tui reg general'
GDB_FLAGS += -ex 'connect-qemu'
