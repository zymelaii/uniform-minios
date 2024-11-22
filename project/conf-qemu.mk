# [in] QEMU_ARCH arch of qemu-system-? executable
# [out] QEMU path to qemu-system-? executable
# [in] QEMU_DISPLAY qemu display graphics
# [in] QEMU_MEMORY physical memory conf, in MB
# [out] QEMU_FLAGS options for qemu-system-?

QEMU_ARCH ?= i386
QEMU      ?= qemu-system-$(QEMU_ARCH)

QEMU_DISPLAY ?=
QEMU_MEMORY  ?= 128

QEMU_FLAGS ?=
QEMU_FLAGS += -boot order=c
QEMU_FLAGS += -serial file:$(OBJDIR)serial-$(shell date +%Y%m%d%H%M).log
QEMU_FLAGS += -m $(QEMU_MEMORY)m
QEMU_FLAGS += $(if $(QEMU_DISPLAY),-display $(QEMU_DISPLAY),)
