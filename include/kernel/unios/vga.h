#pragma once

#include <stdint.h>
#include <stddef.h>

#define SCR_WIDTH   80
#define SCR_HEIGHT  25
#define SCR_SIZE    ((SCR_HEIGHT) * (SCR_WIDTH))
#define SCR_BUFSIZE (2 * (SCR_SIZE))
#define SCR_MAXLINE ((SCR_BUFSIZE) / (SCR_WIDTH))

#define BLACK            0x0
#define WHITE            0x7
#define RED              0x4
#define GREEN            0x2
#define BLUE             0x1
#define FLASH            0x80
#define BRIGHT           0x08
#define MAKE_COLOR(x, y) ((x << 4) | y)

#define FLASH_CHAR         0x8000
#define DEFAULT_CHAR_COLOR ((MAKE_COLOR(BLACK, WHITE | BRIGHT)) << 8)
#define GRAY_CHAR          (MAKE_COLOR(BLACK, BLACK) | BRIGHT)
#define RED_CHAR           (MAKE_COLOR(BLUE, RED) | BRIGHT)
#define WHITE_CHAR         (MAKE_COLOR(BLACK, WHITE | BRIGHT))
#define MAKE_CELL(clr, ch) (clr | ch)
#define BLANK              MAKE_CELL(DEFAULT_CHAR_COLOR, ' ')

#define CRTC_ADDR_REG  0x3d4   //<! CRT Controller Registers - Addr Register
#define CRTC_DATA_REG  0x3d5   //<! CRT Controller Registers - Data Register
#define UNDERLINE_REG  0x14    //<! reg index of underline
#define CURSOR_SHAPE_H 0xa     //<! reg index of cursor shape (MSB)
#define CURSOR_SHAPE_L 0xb     //<! reg index of cursor shape (LSB)
#define START_ADDR_H   0xc     //<! reg index of video mem start addr (MSB)
#define START_ADDR_L   0xd     //<! reg index of video mem start addr (LSB)
#define CURSOR_H       0xe     //<! reg index of cursor position (MSB)
#define CURSOR_L       0xf     //<! reg index of cursor position (LSB)
#define V_MEM_BASE     0xb8000 //<! base of color video memory
#define V_MEM_SIZE     0x8000  //<! 32K: B8000H -> BFFFFH

void vga_wait_for_vrs();

void vga_set_video_start_addr(u32 addr);
void vga_enable_cursor(u8 cur_start, u8 cur_end);
void vga_disable_cursor();
void vga_set_cursor(u32 pos);

void vga_clear_screen();

void vga_set_disppos(int new_pos);
int  vga_get_disppos();

void vga_put_raw(u32 pos, u16 dat);
void vga_flush_blankline(int line_no);
void vga_write_char(const char ch, u16 color);
void vga_write_str(const char *str);
void vga_write_colored_str(const char *str, u8 color);

void vga_copy(u32 dst, u32 src, size_t size);
