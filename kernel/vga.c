#include <unios/vga.h>
#include <unios/layout.h>
#include <arch/x86.h>
#include <string.h>

static int disp_pos = 0;

void vga_wait_for_vrs() {
    while (!(inb(CRTC_ADDR_REG) & (1 << 3))) {}
}

void vga_set_video_start_addr(u32 addr) {
    disable_int();
    outb(CRTC_ADDR_REG, START_ADDR_H);
    outb(CRTC_DATA_REG, (addr >> 8) & 0xff);
    outb(CRTC_ADDR_REG, START_ADDR_L);
    outb(CRTC_DATA_REG, (addr >> 0) & 0xff);
    vga_wait_for_vrs();
    enable_int();
}

void vga_enable_cursor(u8 cur_start, u8 cur_end) {
    disable_int();
    outb(CRTC_ADDR_REG, 0x0a);
    outb(CRTC_DATA_REG, (inb(CRTC_DATA_REG) & 0xc0) | cur_start);
    outb(CRTC_ADDR_REG, 0x0b);
    outb(CRTC_DATA_REG, (inb(CRTC_DATA_REG) & 0xe0) | cur_end);
    vga_wait_for_vrs();
    enable_int();
}

void vga_disable_cursor() {
    disable_int();
    outb(CRTC_ADDR_REG, 0x0a);
    outb(CRTC_DATA_REG, 0x20);
    vga_wait_for_vrs();
    enable_int();
}

void vga_set_cursor(u32 pos) {
    disable_int();
    outb(CRTC_ADDR_REG, CURSOR_H);
    outb(CRTC_DATA_REG, (pos >> 8) & 0xff);
    outb(CRTC_ADDR_REG, CURSOR_L);
    outb(CRTC_DATA_REG, (pos >> 0) & 0xff);
    vga_wait_for_vrs();
    enable_int();
}

void vga_clear_screen() {
    u16 *ptr = K_PHY2LIN(V_MEM_BASE);
    u16 *end = ptr + SCR_WIDTH * SCR_HEIGHT;
    while (ptr != end) { *ptr++ = BLANK; }
    disp_pos = 0;
    vga_set_cursor(disp_pos);
}

void vga_put_raw(u32 pos, u16 dat) {
    *(u16 *)K_PHY2LIN(V_MEM_BASE + pos) = dat;
}

void vga_flush_blankline(int line_no) {
    u32 *dst = K_PHY2LIN(V_MEM_BASE + line_no * SCR_WIDTH * 2);
    for (int i = 0; i < SCR_WIDTH * sizeof(u16) / sizeof(u32); ++i) {
        *dst++ = (BLANK << 16) | BLANK;
    }
}

void vga_set_disppos(int new_pos) {
    disp_pos = new_pos;
}

int vga_get_disppos() {
    return disp_pos;
}

void vga_write_char(char ch, u16 color) {
    if (ch == '\n') {
        disp_pos = (disp_pos / SCR_WIDTH + 1) * SCR_WIDTH;
    } else {
        vga_put_raw(disp_pos * 2, (color << 8) | (u16)ch);
        disp_pos++;
    }
    vga_set_cursor(disp_pos);
}

void vga_write_str(const char *str) {
    for (char *s = (char *)str; *s; ++s) { vga_write_char(*s, WHITE_CHAR); }
}

void vga_write_colored_str(const char *str, u8 color) {
    for (char *s = (char *)str; *s; ++s) { vga_write_char(*s, color); }
}

void vga_copy(u32 dst, u32 src, size_t size) {
    memcpy(
        K_PHY2LIN(V_MEM_BASE + (dst << 1)),
        K_PHY2LIN(V_MEM_BASE + (src << 1)),
        size << 1);
}
