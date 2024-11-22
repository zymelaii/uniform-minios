# [in] PREFIX prefix of the toolchain
# [in] DEFINES definations
# [in] INCDIRS include directories
# [in] LINKDIRS link directories
# [out] CC c compiler executable
# [out] AS assembler executable
# [out] AR ar executable
# [out] LD ld executable
# [out] NM nm executable
# [out] OBJCOPY objcopy executable
# [out] OBJDUMP objdump executable
# [in, out] CFLAGS flags for compiler
# [in, out] ASFLAGS flags for assembler
# [in, out] LDFLAGS flags for linker

PREFIX ?=

CC      := $(PREFIX)gcc
AS      := nasm
AR      := $(PREFIX)ar
LD      := $(PREFIX)ld
NM      := $(PREFIX)nm
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump

DEFINES  ?=
INCDIRS  ?=
INCDIRS  += $(GENERATED_INCDIR)
LINKDIRS ?=

CFLAGS ?=
CFLAGS += $(addprefix -D,$(DEFINES))
CFLAGS += $(addprefix -I,$(INCDIRS)) -MD
CFLAGS += $(addprefix -L,$(LINKDIRS))
CFLAGS += -fno-stack-protector
CFLAGS += -O0
CFLAGS += -std=gnu99
CFLAGS += -fno-builtin -nostdinc -nostdlib
CFLAGS += -m32
CFLAGS += -static -fno-pie
CFLAGS += -g
CFLAGS += -Werror -Wno-unused-variable -Wno-unknown-attributes
#! NOTE: default qemu cpu core may not support sse
CFLAGS += -mno-sse -mno-sse2

ASFLAGS ?=
ASFLAGS += -Iinclude
ASFLAGS += -Ikernel
ASFLAGS += -Iboot/include
ASFLAGS += -I$(GENERATED_INCDIR)

LDFLAGS ?=
LDFLAGS += -m elf_i386
LDFLAGS += -nostdlib
LDFLAGS += $(addprefix -L,$(LINKDIRS))
