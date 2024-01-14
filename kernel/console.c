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
        assert(con != NULL);
        out_char(con, *p == '?' ? nr_tty + '0' : *p);
    }

    vga_set_cursor(con->cursor);
}

static void clear_screen(int pos, int len) {
    u16* p = K_PHY2LIN(V_MEM_BASE + (u32)pos * 2);
    while (--len >= 0) { *p++ = BLANK; }
}

void scroll_screen(console_t* con, int dir, int next_cur_pos) {
    int oldest;
    int newest;
    int scr_top;

    newest  = (next_cur_pos - con->orig) / SCR_WIDTH * SCR_WIDTH;
    oldest  = con->is_full ? (newest + SCR_WIDTH) % con->con_size : 0;
    scr_top = con->crtc_start - con->orig;

    if (dir == SCROLL_DOWN) {
        if (!con->is_full && scr_top > 0) {
            con->crtc_start -= SCR_WIDTH;
        } else if (con->is_full && scr_top != oldest) {
            if (next_cur_pos - con->orig >= con->con_size - SCR_SIZE) {
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
    assert(con != NULL);
    int curr_cursor_x = (con->cursor - con->orig) % SCR_WIDTH;
    int curr_cursor_y = (con->cursor - con->orig) / SCR_WIDTH;
    int curr_curs_pos = con->cursor;

    int next_cursor_x = -1;
    int next_cursor_y = -1;
    int next_curs_pos = -1;

    char disp_ch = 0;

    switch (ch) {
        case '\n': {
            //! no out char
            if (con->wrapped) {
                next_curs_pos = con->orig + SCR_WIDTH * curr_cursor_y;
            } else {
                next_curs_pos = con->orig + SCR_WIDTH * (curr_cursor_y + 1);
            }
        } break;
        case '\r': {
            if (con->wrapped) {
                next_curs_pos = con->orig + (curr_cursor_y - 1) * SCR_WIDTH;
            } else {
                next_curs_pos = con->orig + curr_cursor_y * SCR_WIDTH;
            }
        } break;
        case '\b': {
            disp_ch       = ' ';
            next_curs_pos = curr_curs_pos - 1;
            --curr_curs_pos;
        } break;
        default: {
            disp_ch       = ch;
            next_curs_pos = curr_curs_pos + 1;
        } break;
    }
    next_cursor_x = (next_curs_pos - con->orig) % SCR_WIDTH;
    next_cursor_y = (next_curs_pos - con->orig) / SCR_WIDTH;

    if (ch == '\r' || ch == '\n') {
        con->wrapped = false;
    } else if ((curr_curs_pos + 1 - con->orig) / SCR_WIDTH > curr_cursor_y) {
        con->wrapped = true;
    } else if ((next_curs_pos - 1 - con->orig) / SCR_WIDTH < curr_cursor_y) {
        con->wrapped = true;
    } else if (ch != '\n' && ch != '\r' && con->wrapped == true) {
        con->wrapped = false;
    }
    assert(next_curs_pos != -1);

    if (next_curs_pos - con->orig >= con->con_size - SCR_WIDTH - 1) {
        int cp_orig = con->orig + (next_cursor_y + 1) * SCR_WIDTH - SCR_SIZE;
        disable_int();
        vga_copy(con->orig, cp_orig, SCR_SIZE - SCR_WIDTH);
        enable_int();
        con->crtc_start = con->orig;
        next_curs_pos   = con->orig + (SCR_SIZE - SCR_WIDTH) + next_cursor_x;
        clear_screen(
            next_curs_pos, con->con_size - (next_curs_pos - con->orig));
        if (!con->is_full) con->is_full = 1;
    }

    while (next_curs_pos >= con->crtc_start + SCR_SIZE
           || next_curs_pos < con->crtc_start) {
        scroll_screen(con, SCROLL_UP, next_curs_pos);
        clear_screen(next_curs_pos, SCR_WIDTH);
    }
    if (disp_ch != 0) {
        vga_set_disppos(curr_curs_pos);
        vga_write_char(disp_ch, WHITE_CHAR);
    }

    con->cursor = next_curs_pos;

    assert(con->cursor - con->orig < con->con_size);
    assert(con->cursor - con->crtc_start < 1920);
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
