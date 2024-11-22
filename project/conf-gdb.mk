# [in, out] GDB gdb executable
# [in] GDB_SCRIPTS_HOME path to user-defined gdb scripts
# [out] GDB_FLAGS options for gdb

GDB ?= gdb

GDB_SCRIPTS_HOME    ?=
GDB_REALMODE_XML    := $(GDB_SCRIPTS_HOME)target.xml
GDB_REALMODE_SCRIPT := $(GDB_SCRIPTS_HOME)gdb_init_real_mode.txt

GDB_FLAGS ?=
GDB_FLAGS += -q -nx
GDB_FLAGS += -x '$(GDB_SCRIPTS_HOME)connect-qemu.gdb'
GDB_FLAGS += -x '$(GDB_SCRIPTS_HOME)instr-level.gdb'
GDB_FLAGS += -x '$(GDB_SCRIPTS_HOME)layout.gdb'
GDB_FLAGS += -ex 'set pagination off'
GDB_FLAGS += -ex 'set confirm off'
GDB_FLAGS += -ex 'set disassembly-flavor intel'
GDB_FLAGS += -ex 'connect-qemu'
GDB_FLAGS += -ex 'focus cmd'
