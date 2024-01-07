#pragma once

#include <stdint.h>
#include <stdbool.h>

//! whether the kernel is being initialized
extern bool kstate_on_init;

//! reenter times in kernel
extern int kstate_reenter_cntr;
