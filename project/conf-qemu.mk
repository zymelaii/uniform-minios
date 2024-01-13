# [in, out] QEMU qemu-system-? executable
# [in] QEMU qemu display graphics
# [out] QEMU_FLAGS options for qemu-system-?

QEMU    ?= qemu-system-i386
DISPLAY :=

QEMU_FLAGS := -boot order=a
QEMU_FLAGS += -serial file:serial.log
QEMU_FLAGS += -m 128m
QEMU_FLAGS += $(if $(DISPLAY),-display $(DISPLAY),)
