#pragma once

//! NOTE: ref http://www.osdever.net/bkerndev/Docs/pit.htm

//! 8253/8254 PIT
#define TIMER0     0x40    //<! timer channel 0
#define TIMER_MODE 0x43    //<! timer mode control
#define TIMER_FREQ 1193182 //<! PIT frequency
//! 00-11-010-0 : Counter0 - LSB then MSB - rate generator - binary
#define RATE_GENERATOR 0x34

void init_sysclk();

extern int system_ticks;
