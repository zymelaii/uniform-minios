#pragma once

#include <unios/vga.h>
#include <unios/sync.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct console_s {
    vga_textmode_state_t state;
    void                *vmem_real;
    void                *vmem_buf;
    spinlock_t           wrlock;
} console_t;

void setup_vga_console(console_t *con);
bool vcon_is_foreground(console_t *con);
bool vcon_is_offscreen(console_t *con);
void vcon_make_foreground(console_t *con);
void vcon_make_offscreen(console_t *con);
void vcon_scroll(console_t *con, int row_diff);
void vcon_write(console_t *con, const char *buf, int size);
void vcon_clear_screen(console_t *con);
