#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/tty.h>
#include <unios/console.h>
#include <unios/keyboard.h>
#include <unios/assert.h>
#include <unios/vga.h>
#include <unios/console.h>
#include <sys/defs.h>
#include <arch/x86.h>
#include <string.h>

#define TTY_FIRST (tty_table)
#define TTY_END   (tty_table + NR_CONSOLES)

int   current_console;
tty_t tty_table[NR_CONSOLES];

static void flush_screen_scroll(tty_t *tty, int expected_cur_line) {
    assert(expected_cur_line == -1 || expected_cur_line >= 0);
    if (expected_cur_line != -1) {
        tty->console->current_line = expected_cur_line;
    }
    int cur_line   = tty->console->current_line;
    int real_line  = tty->console->orig / SCR_WIDTH;
    int vga_offset = SCR_WIDTH * (cur_line + real_line);
    vga_set_video_start_addr(vga_offset);
}

static void flush_cursor_pos(int pos) {
    vga_set_cursor(pos);
}

static void tty_init(tty_t *tty) {
    tty->status    = TTY_DISPLAY;
    tty->ibuf_next = tty->ibuf;
    tty->ibuf_wr   = tty->ibuf_next;
    tty->cnt_wr    = 0;
    tty->ibuf_rd   = tty->ibuf_next;
    tty->cnt_rd    = 0;

    int nr_tty   = tty - tty_table;
    tty->console = &console_table[nr_tty];

    memset(&tty->mouse, 0, sizeof(tty->mouse));

    init_screen(tty);
}

static bool tty_buf_push(tty_t *tty, u32 code) {
    if (tty->cnt_wr >= TTY_BUFSZ) { return false; }
    *tty->ibuf_next = code;
    tty->ibuf_next  = &tty->ibuf[(tty->ibuf_next - tty->ibuf + 1) % TTY_BUFSZ];
    ++tty->cnt_wr;
    ++tty->cnt_rd;
    return true;
}

static void tty_buf_pop(tty_t *tty) {
    if (tty->cnt_rd == 0) {
        assert(tty->cnt_wr == 0);
        return;
    }

    u32 *last      = tty->ibuf_next;
    int  offset    = tty->ibuf_next - tty->ibuf;
    tty->ibuf_next = &tty->ibuf[(offset + TTY_BUFSZ - 1) % TTY_BUFSZ];

    if (tty->ibuf_wr == last) {
        assert(tty->cnt_wr == 0);
        tty->ibuf_wr = tty->ibuf_next;
    } else {
        --tty->cnt_wr;
    }

    if (tty->ibuf_rd == last) {
        assert(tty->cnt_rd == 0);
        tty->ibuf_rd = tty->ibuf_next;
    } else {
        --tty->cnt_rd;
    }
}

static bool tty_rdbuf_next(tty_t *tty, u32 *code) {
    if (tty->cnt_rd == 0) {
        assert(tty->ibuf_wr == tty->ibuf_next);
        return false;
    }
    if (code != NULL) { *code = *tty->ibuf_rd; }
    tty->ibuf_rd = &tty->ibuf[(tty->ibuf_rd - tty->ibuf + 1) % TTY_BUFSZ];
    --tty->cnt_rd;
    return true;
}

static bool tty_wrbuf_next(tty_t *tty, u32 *code) {
    if (tty->cnt_wr == 0) {
        assert(tty->ibuf_wr == tty->ibuf_next);
        return false;
    }
    if (code != NULL) { *code = *tty->ibuf_wr; }
    tty->ibuf_wr = &tty->ibuf[(tty->ibuf_wr - tty->ibuf + 1) % TTY_BUFSZ];
    --tty->cnt_wr;
    return true;
}

static bool tty_wrbuf_rewind_once(tty_t *tty) {
    assert(tty->cnt_rd >= tty->cnt_wr);
    if (tty->cnt_rd == tty->cnt_wr) { return false; }
    int offset   = tty->ibuf_wr - tty->ibuf;
    tty->ibuf_wr = &tty->ibuf[(offset + TTY_BUFSZ - 1) % TTY_BUFSZ];
    ++tty->cnt_wr;
    return true;
}

static bool tty_wrbuf_erase_first(tty_t *tty) {
    if (tty->cnt_wr == 0) {
        assert(tty->ibuf_wr == tty->ibuf_next);
        return false;
    }

    int ip  = tty->ibuf_wr - tty->ibuf;
    int iq  = (ip + 1) % TTY_BUFSZ;
    int end = tty->ibuf_next - tty->ibuf;
    while (iq != end) {
        tty->ibuf[ip] = tty->ibuf[iq];
        ip            = iq;
        iq            = (iq + 1) % TTY_BUFSZ;
    }

    tty_buf_pop(tty);
    return true;
}

//! put one key to the target tty
static void tty_put_key(tty_t *tty, u32 key) {
    bool ok = tty_buf_push(tty, key);
    assert(ok && "not enough space in tty buffer");
}

//! NOTE: currently only support console stdout
//! TODO: support general tty write, e.g. serial write
void tty_write(tty_t *tty, char *buf, int len) {
    for (int i = 0; i < len; ++i) { out_char(tty->console, buf[i]); }
}

//! NOTE: currently only support console stdin
//! TODO: support general tty read, e.g. serial read
int tty_read(tty_t *tty, char *buf, int len) {
    assert(tty != NULL);
    assert(buf != NULL);
    assert(len >= 0);

    int total_rd = 0;
    u32 code     = 0;

    while (total_rd < len) {
        if (tty->cnt_rd == 0) { tty->status |= TTY_WAIT_ENTER; }
        while (tty->status & TTY_WAIT_ENTER) {}
        while (tty->cnt_rd > 0) {
            bool ok = tty_rdbuf_next(tty, &code);
            assert(ok);
            buf[total_rd++] = code;
            if (total_rd >= len) { break; }
        }
    }

    assert(total_rd == len);
    return total_rd;
}

void tty_keyboard_proc(tty_t *tty, u32 key) {
    if (!(key & FLAG_EXT)) {
        tty_put_key(tty, key);
        return;
    }

    int real_line = tty->console->orig / SCR_WIDTH;
    int raw_code  = key & MASK_RAW;

    do {
        if (raw_code == ENTER) {
            tty_put_key(tty, '\n');
            tty->status &= ~TTY_WAIT_ENTER;
        } else if (raw_code == BACKSPACE) {
            tty_put_key(tty, '\b');
        } else if (raw_code == UP) {
            //! FIXME: why constant 43?
            if (tty->console->current_line >= 43) { break; }
            flush_screen_scroll(tty, tty->console->current_line + 1);
        } else if (raw_code == DOWN) {
            if (tty->console->current_line == 0) { break; }
            flush_screen_scroll(tty, tty->console->current_line - 1);
        } else if (raw_code >= F1 && raw_code <= F12) {
            //! NOTE: assume F1~F12 is consistent
            int nr_console = raw_code - F1;
            if (nr_console >= 0 && nr_console < NR_CONSOLES) {
                select_console(nr_console);
            }
        }
    } while (0);

    //! TODO: other cases
}

static void tty_mouse(tty_t *tty) {
    if (!is_current_console(tty->console)) { return; }

    // scroll up: left-button up & drag up
    if ((tty->mouse.buttons & MOUSE_LEFT_BUTTON)
        && tty->mouse.off_y > MOUSE_UPDOWN_BOUND) {
        if (tty->console->current_line < 43) {
            flush_screen_scroll(tty, tty->console->current_line + 1);
            tty->mouse.off_y = 0;
        }
    }

    // scroll down: left-button down & drag down
    if ((tty->mouse.buttons & MOUSE_LEFT_BUTTON)
        && tty->mouse.off_y < -MOUSE_UPDOWN_BOUND) {
        if (tty->console->current_line > 0) {
            flush_screen_scroll(tty, tty->console->current_line - 1);
            tty->mouse.off_y = 0;
        }
    }

    // resume: middle-button down
    if (tty->mouse.buttons & MOUSE_MIDDLE_BUTTON) {
        flush_screen_scroll(tty, 0);
        tty->mouse.off_y = 0;
    }
}

//! read from the device to the rdbuf
//! TODO: support other devices except for console
static void tty_dev_read(tty_t *tty) {
    if (!is_current_console(tty->console)) { return; }
    keyboard_read(tty);
}

//! flush wrbuf to the device
static void tty_dev_write(tty_t *tty) {
    assert(tty != NULL);

    //! represent an invalid code
    const u32 NONE      = ~0;
    u32       code      = NONE;
    u32       prev_code = NONE;

    //! rewind to get the prev code
    bool ok = tty_wrbuf_rewind_once(tty);
    if (ok) {
        ok = tty_wrbuf_next(tty, &code);
        assert(ok);
    }

    //! NOTE: actually, size of wrbuf always tends to be 1~2
    while (true) {
        prev_code = code;
        ok        = tty_wrbuf_next(tty, &code);
        if (!ok) { break; }

        bool should_output = true;

        //! TODO: lower the time complexity
        if (code == '\b') {
            ok = tty_wrbuf_rewind_once(tty);
            assert(ok);
            ok = tty_wrbuf_erase_first(tty);
            assert(ok);
            //! NOTE: '\n' will move the cursor to the newline, but i do not
            //! save the pos history, thus when consume a '\b' following a
            //! '\n', only eat the '\b' and do not remove the previous '\n'
            if (prev_code == NONE || prev_code == '\n' || prev_code == '\b') {
                should_output = false;
            } else {
                ok = tty_wrbuf_rewind_once(tty);
                assert(ok);
                ok = tty_wrbuf_erase_first(tty);
                assert(ok);
            }
        }

        if (should_output) { out_char(tty->console, code); }
    }
}

void tty_handler() {
    tty_t *tty = NULL;
        for (tty = TTY_FIRST; tty < TTY_END; ++tty) { tty_init(tty); }

    //! select first tty & console
    tty = TTY_FIRST;
    select_console(0);

    //! reset cursor pos to be safe
    flush_cursor_pos(vga_get_disppos());

    while (true) {
        for (tty = TTY_FIRST; tty < TTY_END; ++tty) {
            //! NOTE: always try recv before handle
            //! NOTE: actually this is expected to be a dead loop, here we break
            //! the loop only to free up the cpu at a convenient time to allow
            //! other ttys handle its own jobs, that is, this is just a very
            //! simple schedule program
            do {
                tty_mouse(tty);
                tty_dev_read(tty);
                tty_dev_write(tty);
            } while (tty->cnt_wr > 0);
        }
    }
}
