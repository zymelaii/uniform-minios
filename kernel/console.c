#include <unios/console.h>
#include <unios/vga.h>
#include <unios/tty.h>
#include <unios/layout.h>
#include <unios/assert.h>
#include <arch/x86.h>
#include <sys/defs.h>
#include <string.h>
#include <stdio.h>

console_t console_table[NR_CONSOLES];

static void flush(console_t* con) {
    if (is_current_console(con)) {
        vga_set_cursor(con->cursor);
        vga_set_video_start_addr(con->crtc_start);
    }
}

void init_screen(tty_t* tty) {
    int v_mem_size   = V_MEM_SIZE >> 1;
    int size_per_con = (v_mem_size / NR_CONSOLES) / SCR_WIDTH * SCR_WIDTH;

    int nr_tty   = tty - tty_table;
    tty->console = &console_table[nr_tty];

    console_t* con    = tty->console;
    con->crtc_start   = nr_tty * size_per_con;
    con->orig         = con->crtc_start;
    con->con_size     = size_per_con;
    con->cursor       = con->orig;
    con->is_full      = false;
    con->current_line = 0;

    if (nr_tty == 0) { con->cursor += vga_get_disppos(); }
    const char prompt[] = "[TTY #?]\n";
    for (const char* p = prompt; *p != '\0'; ++p) {
        out_char(con, *p == '?' ? nr_tty + '0' : *p);
    }

    vga_set_cursor(con->cursor);
    trace_logging("tty %d cursor: %d\n", nr_tty, con->cursor);
}

static void clear_screen(int pos, int len) {
    u16* p = K_PHY2LIN(V_MEM_BASE + (u32)pos * 2);
    while (--len >= 0) { *p++ = BLANK; }
}

void scroll_screen(console_t* con, int dir) {
    int oldest;
    int newest;
    int scr_top;

    newest  = (con->cursor - con->orig) / SCR_WIDTH * SCR_WIDTH;
    oldest  = con->is_full ? (newest + SCR_WIDTH) % con->con_size : 0;
    scr_top = con->crtc_start - con->orig;

    if (dir == SCROLL_DOWN) {
        if (!con->is_full && scr_top > 0) {
            con->crtc_start -= SCR_WIDTH;
        } else if (con->is_full && scr_top != oldest) {
            if (con->cursor - con->orig >= con->con_size - SCR_SIZE) {
                if (con->crtc_start != con->orig) con->crtc_start -= SCR_WIDTH;
            } else if (con->crtc_start == con->orig) {
                scr_top         = con->con_size - SCR_SIZE;
                con->crtc_start = con->orig + scr_top;
            } else {
                con->crtc_start -= SCR_WIDTH;
            }
        }
    } else if (dir == SCROLL_UP) {
        if (!con->is_full && newest >= scr_top + SCR_SIZE) {
            con->crtc_start += SCR_WIDTH;
        } else if (con->is_full && scr_top + SCR_SIZE - SCR_WIDTH != newest) {
            if (scr_top + SCR_SIZE == con->con_size)
                con->crtc_start = con->orig;
            else
                con->crtc_start += SCR_WIDTH;
        }
    } else {
        assert(dir == SCROLL_DOWN || dir == SCROLL_UP);
    }

    flush(con);
}

void out_char(console_t* con, char ch) {
    //! FIXME: dirty current_line
    int cursor_x = (con->cursor - con->orig) % SCR_WIDTH;
    int cursor_y = (con->cursor - con->orig) / SCR_WIDTH;

    switch (ch) {
        case '\n': {
            con->cursor = con->orig + SCR_WIDTH * (cursor_y + 1);
            vga_set_disppos(con->cursor);
        } break;
        case '\b': {
            if (con->cursor > con->orig) {
                --con->cursor;
                vga_set_disppos(con->cursor);
                vga_write_char(' ', WHITE_CHAR);
            }
        } break;
        default: {
            vga_set_disppos(con->cursor);
            vga_write_char(ch, WHITE_CHAR);
            ++con->cursor;
        } break;
    }

    if (con->cursor - con->orig >= con->con_size) {
        cursor_x    = (con->cursor - con->orig) % SCR_WIDTH;
        cursor_y    = (con->cursor - con->orig) / SCR_WIDTH;
        int cp_orig = con->orig + (cursor_y + 1) * SCR_WIDTH - SCR_SIZE;
        vga_copy(con->orig, cp_orig, SCR_SIZE - SCR_WIDTH);
        con->crtc_start = con->orig;
        con->cursor     = con->orig + (SCR_SIZE - SCR_WIDTH) + cursor_x;
        clear_screen(con->cursor, SCR_WIDTH);
        if (!con->is_full) con->is_full = 1;
    }

    assert(con->cursor - con->orig < con->con_size);

    while (con->cursor >= con->crtc_start + SCR_SIZE
           || con->cursor < con->crtc_start) {
        scroll_screen(con, SCROLL_UP);
        clear_screen(con->cursor, SCR_WIDTH);
    }

    flush(con);
}

int is_current_console(console_t* con) {
    return (con == &console_table[current_console]);
}

void select_console(int nr_console) {
    if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) { return; }
    current_console = nr_console;
    flush(&console_table[current_console]);
}
