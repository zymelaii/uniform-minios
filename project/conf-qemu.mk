# [in, out] QEMU qemu-system-? executable
# [in] QEMU_DISPLAY qemu display graphics
# [in] MEMORY physical memory conf, in MB
# [out] QEMU_FLAGS options for qemu-system-?

QEMU    ?= qemu-system-i386

QEMU_DISPLAY :=
MEMORY       := 128

QEMU_FLAGS := -boot order=a
QEMU_FLAGS += -serial file:serial.log
QEMU_FLAGS += -m $(MEMORY)m
QEMU_FLAGS += $(if $(QEMU_DISPLAY),-display $(QEMU_DISPLAY),)
