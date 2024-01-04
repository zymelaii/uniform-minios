# [in, out] QEMU qemu-system-? executable
# [out] QEMU_FLAGS options for qemu-system-?

QEMU ?= qemu-system-i386

QEMU_FLAGS := -boot order=a
QEMU_FLAGS += -serial file:serial.log
QEMU_FLAGS += -m 128m
QEMU_FLAGS += -display curses
