# path to project mk files
PROJMK_PREFIX ?=

# configure output dir
OBJDIR ?= build/

# uniform-os image
IMAGE_NAME ?= unios

# uniform-os kernel
KERNEL_NAME ?= unikrnl

# standard library for uniform-os
LIBRT ?= unirt

# archive of user programs
USER_PROG_ARCHIVE ?= app.tar

include $(PROJMK_PREFIX)conf-unios.mk
