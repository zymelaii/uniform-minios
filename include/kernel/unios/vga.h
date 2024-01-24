#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

//! NOTE: see ref. sites as below
//! http://www.osdever.net/FreeVGA/vga/vga.htm
//! http://www.osdever.net/FreeVGA/vga/graphreg.htm
//! http://www.osdever.net/FreeVGA/vga/crtcreg.htm
//! http://www.osdever.net/documents/vga_ports.txt

#define VGA_VMEM0_BASE 0xa0000 //<! memory map select 00b
#define VGA_VMEM0_SIZE 0x20000 //<! 128 KB
#define VGA_VMEM1_BASE 0xa0000 //<! memory map select 01b
#define VGA_VMEM1_SIZE 0x10000 //<! 64 KB
#define VGA_VMEM2_BASE 0xb0000 //<! memory map select 10b
#define VGA_VMEM2_SIZE 0x08000 //<! 32 KB
#define VGA_VMEM3_BASE 0xb8000 //<! memory map select 11b
#define VGA_VMEM3_SIZE 0x08000 //<! 32 KB

//! vga registers
#define VGA_REG_MISC        0x3c2 //<! miscellaneous register
#define VGA_REG_SEQ_INDEX   0x3c4 //<! sequencer index register
#define VGA_REG_SEQ_DATA    0x3c5 //<! sequencer data register
#define VGA_REG_CRTC_INDEX0 0x3d4 //<! CRT controller index register 0
#define VGA_REG_CRTC_DATA0  0x3d5 //<! CRT controller data register 0
#define VGA_REG_CRTC_INDEX1 0x3b4 //<! CRT controller index register 1
#define VGA_REG_CRTC_DATA1  0x3b5 //<! CRT controller data register 1
#define VGA_REG_GC_INDEX    0x3ce //<! graphics controller index register
#define VGA_REG_GC_DATA     0x3cf //<! graphics controller data register
#define VGA_REG_AC_INDEX    0x3c0 //<! attribute controller index register
#define VGA_REG_AC_DATA     0x3c1 //<! attribute controller data register

//! vga crt controller index
#define VGA_CRTC_HTOTAL         0x00 //<! horizontal total
#define VGA_CRTC_HDISPLAY_END   0x01 //<! horizontal display enable end
#define VGA_CRTC_HBLANK_START   0x02 //<! start horizontal blanking
#define VGA_CRTC_HBLANK_END     0x03 //<! end horizontal blanking
#define VGA_CRTC_HRETRACE_START 0x04 //<! start horizontal retrace
#define VGA_CRTC_HRETRACE_END   0x05 //<! end horizontal retrace
#define VGA_CRTC_VTOTAL         0x06 //<! vertical total
#define VGA_CRTC_OVERFLOW       0x07 //<! overflow
#define VGA_CRTC_MAX_SCANLINE   0x09 //<! max scanline
#define VGA_CRTC_CURSOR_START   0x0a //<! cursor start
#define VGA_CRTC_CURSOR_END     0x0b //<! cursor end
#define VGA_CRTC_START_ADDR_HI  0x0c //<! video memory start addr, hi
#define VGA_CRTC_START_ADDR_LO  0x0d //<! video memory start addr, lo
#define VGA_CRTC_CURSOR_LOC_HI  0x0e //<! cursor location, hi
#define VGA_CRTC_CURSOR_LOC_LO  0x0f //<! cursor location, lo
#define VGA_CRTC_VRETRACE_START 0x10 //<! vertical retrace start
#define VGA_CRTC_VRETRACE_END   0x11 //<! vertical retrace end
#define VGA_CRTC_VDISPLAY_END   0x12 //<! vertical display enable end
#define VGA_CRTC_OFFSET         0x13 //<! offset
#define VGA_CRTC_MODECTL        0x17 //<! mode control

uint8_t vga_get_crtc_data_unsafe(int index);
void    vga_set_crtc_data_unsafe(int index, uint8_t data);
uint8_t vga_get_crtc_data_direct_unsafe();
void    vga_set_crtc_data_direct_unsafe(uint8_t data);

uint8_t  vga_get_textmode_char_height_unsafe();
uint8_t  vga_get_textmode_width_unsafe();
uint8_t  vga_get_textmode_height_unsafe();
uint16_t vga_get_display_end_scanline_unsafe();

void vga_set_vmem_base_unsafe(uint16_t addr);
void vga_set_cursor_shape_unsafe(uint8_t start_scanline, uint8_t end_scanline);
void vga_set_cursor_skew_unsafe(uint8_t skew);
void vga_set_cursor_visible_unsafe(bool visible);
void vga_set_cursor_pos_unsafe(uint16_t pos);

typedef union vga_textmode_char_u {
    uint16_t raw;

    struct {
        uint8_t code;
        uint8_t attr;
    };
} vga_textmode_char_t;

typedef struct vga_textmode_state_s {
    //! screen width
    uint16_t width;
    //! screen height
    uint16_t height;
    //! total size of vmem
    uint32_t capacity;
    //! vmem base phy addr
    phyaddr_t vmem_base_phy;
    //! vmem base addr
    void *vmem_base;
    //! addr of screen origin
    vga_textmode_char_t *origin;
    //! where is the last written char, include cntrl char, relative to `origin`
    int32_t last_char_pos;
    //! where to write the next char, relative to `origin`
    int32_t next_char_pos;
    //! where is the cursor, indicate the current line, relative to `origin`
    uint16_t cursor_pos;
    //! real number of a wrapped line, relative to `origin`
    int16_t real_line;
    //! attr since the last update
    uint8_t last_attr;
    //! cursor shape: begin scan line
    uint8_t cursor_begin;
    //! cursor shape: end scan line
    uint8_t cursor_end;
    //! whether cursor is enabled
    bool cursor_enabled;
    //! whether to use the `last_attr` or the old attr from the current loc
    bool use_last_attr;
} vga_textmode_state_t;

//! vga textmode methods
//! NOTE: some methods have very similar cfgs, but we implement them separately
//! to reduce the cost of fncalls
void vgatm_switch_to_unsafe(vga_textmode_state_t *state);
void vgatm_sync_screen_unsafe(vga_textmode_state_t *state);
void vgatm_update_cursor_pos_unsafe(vga_textmode_state_t *state);
void vgatm_sync_cursor_unsafe(vga_textmode_state_t *state);
void vgatm_flush_vmem(vga_textmode_state_t *state, uint16_t raw);
void vgatm_fill_screen(vga_textmode_state_t *state, uint16_t raw);
void vgatm_fill_line(
    vga_textmode_state_t *state, uint16_t lineno, uint16_t raw);
void vgatm_fill_absolute_line(
    vga_textmode_state_t *state, uint32_t lineno, uint16_t raw);
void vgatm_fill_current_line(vga_textmode_state_t *state, uint16_t raw);
void vgatm_fill_current_wrapped_line(vga_textmode_state_t *state, uint16_t raw);
void vgatm_write_printable_char_direct(vga_textmode_state_t *state, char ch);
void vgatm_write_printable_char_at_direct_unsafe(
    vga_textmode_state_t *state, int32_t pos, char ch);
void vgatm_put_printable_char_direct(vga_textmode_state_t *state, char ch);
void vgatm_put_printable_str_direct(
    vga_textmode_state_t *state, const char *str);
void vgatm_write_raw(vga_textmode_state_t *state, uint16_t raw);
void vgatm_write_raw_at_unsafe(
    vga_textmode_state_t *state, int32_t pos, uint16_t raw);
void vgatm_put_raw(vga_textmode_state_t *state, uint16_t raw);
void vgatm_accept_ctrl_lf(vga_textmode_state_t *state);
void vgatm_accept_ctrl_cr(vga_textmode_state_t *state);
void vgatm_accept_ctrl_bs(vga_textmode_state_t *state);
void vgatm_accept_ctrl_ff(vga_textmode_state_t *state);
bool vgatm_accept_char(vga_textmode_state_t *state, char ch);
int  vgatm_accept_str_anyhow(vga_textmode_state_t *state, const char *str);
int  vgatm_accept_str_until_fail(vga_textmode_state_t *state, const char *str);
int  vgatm_accept_csi_seq(vga_textmode_state_t *state, const char *seq);

//! vga textmode attribute
#define VGATMCLR_black      0
#define VGATMCLR_blue       1
#define VGATMCLR_green      2
#define VGATMCLR_cyan       3
#define VGATMCLR_red        4
#define VGATMCLR_magenta    5
#define VGATMCLR_brown      6
#define VGATMCLR_lightgray  7
#define VGATMCLR_darkgray   8
#define VGATMCLR_lightblue  9
#define VGATMCLR_lightgreen 10
#define VGATMCLR_lightcyan  11
#define VGATMCLR_lightred   12
#define VGATMCLR_pink       13
#define VGATMCLR_yellow     14
#define VGATMCLR_white      15

#define VGATM_BLINK(code)   (((uint16_t)(code) >> 15) & 1)
#define VGATM_FGCOLOR(code) (((uint16_t)(code) >> 8) & 0xf)
#define VGATM_BGCOLOR(code) (((uint16_t)(code) >> 12) & 0x7)

#define VGATM_ATTR(fg, bg, blink) \
 ((!!(blink) << 7) | ((VGATMCLR_##bg & 0x7) << 4) | VGATMCLR_##fg)
