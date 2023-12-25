#include "tty.h"
#include "x86.h"
#include "console.h"
#include "const.h"
#include "assert.h"

/*****************************************************************************
    Set the position of cursor in the screen
    @param position the position in the linear array of video buffer
*/
static inline void vga_set_cursor(unsigned int position)
{
	disable_int();
	outb(CRTC_ADDR_REG, CURSOR_H);
	outb(CRTC_DATA_REG, (position >> 8) & 0xFF);
	outb(CRTC_ADDR_REG, CURSOR_L);
	outb(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*****************************************************************************
    Disable the blinking cursor
*/
static inline void vga_disable_cursor()
{
	outb(CRTC_ADDR_REG, 0x0A);
	outb(CRTC_DATA_REG, 0x20);
}

/*****************************************************************************
    Enable the blinking cursor, and set the height of the cursor
    The cursor is splited into 16 units
    @param cursor_start the start unit index
    @param cursor_end  the ending unit index
*/
static inline void vga_enable_cursor(u8 cursor_start, u8 cursor_end)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

/*****************************************************************************
    Write data directly to Video Memory cell
    @param pos  text mode position(pos*2 yourself)
    @param dat  data to be written, with format [ BG | FG |  ASCII  ]
*/
static inline void vga_put_raw(u32 pos, u16 dat)
{
	u16 *pch = (u16 *)K_PHY2LIN(V_MEM_BASE + pos);
	*pch = dat;
}

static inline void vga_flush_blankline(int line_no)
{
	u32 *dst = (u32 *)K_PHY2LIN(V_MEM_BASE + line_no * SCR_WIDTH * 2);
	for (int i = 0; i < SCR_WIDTH * sizeof(u16) / sizeof(u32); ++i)
	{
		*dst++ = (BLANK << 16) | BLANK;
	}
}

static int disp_pos = 0;

void vga_set_disppos(int new_pos) {
    disp_pos = new_pos;
}
int vga_get_disppos() {
    return disp_pos;
}

/**********************************************************
 * Write char to disp_pos and then increase disp_pos
 * @param ch the character
 * @param color the color, in 8-bit format
*/
void vga_write_char(const char ch, u16 color) {
    if (ch == '\n') {
        disp_pos = (disp_pos / SCR_WIDTH + 1) * SCR_WIDTH;
    }
    else {
        vga_put_raw(disp_pos * 2, (color << 8) | (u16)ch);
        disp_pos ++;
    }
    vga_set_cursor(disp_pos);
}

void vga_write_str(const char * str) {
    for (char* s = (char *)str; *s; ++s) {
        vga_write_char(*s, WHITE_CHAR);
    }
}

void vga_write_str_color(const char * str, u8 color) {
    for (char* s = (char *)str; *s; ++s) {
        vga_write_char(*s, color);
    }
}