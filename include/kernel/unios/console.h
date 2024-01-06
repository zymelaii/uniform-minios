#pragma once

#include <stddef.h>
#include <stdbool.h>

#define SCROLL_UP   (+1)
#define SCROLL_DOWN (-1)

typedef struct console_s {
    uintptr_t crtc_start; //<! set CRTC start addr reg
    uintptr_t orig;       //<! start addr of the console
    size_t    con_size;   //<! how many words does the console have
    bool      is_full;
    int       cursor;
    int       current_line;
} console_t;

extern int       current_console;
extern console_t console_table[];

void out_char(console_t* con, char ch);
