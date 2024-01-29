#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/tty.h>
#include <unios/console.h>
#include <unios/keyboard.h>
#include <unios/console.h>
#include <unios/tracing.h>
#include <sys/defs.h>
#include <arch/x86.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

tty_t *tty_table[NR_CONSOLES];

static volatile int tty_wait_nr;

static void tty_init(tty_t *tty) {
    //! TODO: move part of struct to user space

    tty->status    = TTY_DISPLAY;
    tty->ibuf_next = tty->ibuf;
    tty->ibuf_wr   = tty->ibuf_next;
    tty->cnt_wr    = 0;
    tty->ibuf_rd   = tty->ibuf_next;
    tty->cnt_rd    = 0;

    console_t *con = kmalloc(sizeof(console_t));
    assert(con != NULL);
    setup_vga_console(con);
    vcon_make_offscreen(con);
    vga_textmode_char_t charcode = {
        .attr = con->state.last_attr,
        .code = ' ',
    };
    vgatm_flush_vmem(&con->state, charcode.raw);
    tty->console = con;

    memset(&tty->mouse, 0, sizeof(tty->mouse));
}

static bool tty_buf_push(tty_t *tty, uint32_t code) {
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

    uint32_t *last   = tty->ibuf_next;
    int       offset = tty->ibuf_next - tty->ibuf;
    tty->ibuf_next   = &tty->ibuf[(offset + TTY_BUFSZ - 1) % TTY_BUFSZ];

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

static bool tty_rdbuf_next(tty_t *tty, uint32_t *code) {
    if (tty->cnt_rd == 0) {
        assert(tty->ibuf_wr == tty->ibuf_next);
        return false;
    }
    if (code != NULL) { *code = *tty->ibuf_rd; }
    tty->ibuf_rd = &tty->ibuf[(tty->ibuf_rd - tty->ibuf + 1) % TTY_BUFSZ];
    --tty->cnt_rd;
    return true;
}

static bool tty_wrbuf_next(tty_t *tty, uint32_t *code) {
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
static void tty_put_key(tty_t *tty, uint32_t key) {
    bool ok = tty_buf_push(tty, key);
    assert(ok && "not enough space in tty buffer");
}

//! NOTE: currently only support console stdout
//! TODO: support general tty write, e.g. serial write
void tty_write(tty_t *tty, char *buf, int len) {
    vcon_write(tty->console, buf, len);
}

//! NOTE: currently only support console stdin
//! TODO: support general tty read, e.g. serial read
int tty_read(tty_t *tty, char *buf, int len) {
    assert(tty != NULL);
    assert(buf != NULL);
    assert(len >= 0);

    int      total_rd = 0;
    uint32_t code     = 0;

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

void tty_keyboard_proc(tty_t *tty, uint32_t key) {
    if (!(key & FLAG_EXT)) {
        tty_put_key(tty, key);
        return;
    }

    int raw_code = key & MASK_RAW;

    if (raw_code == ENTER) {
        tty_put_key(tty, '\n');
        tty->status &= ~TTY_WAIT_ENTER;
    } else if (raw_code == BACKSPACE) {
        tty_put_key(tty, '\b');
    } else if (raw_code == UP) {
        vcon_scroll(tty->console, -1);
    } else if (raw_code == DOWN) {
        vcon_scroll(tty->console, 1);
    } else if (raw_code >= F1 && raw_code <= F12) {
        //! NOTE: assume F1~F12 is consistent
        int index = raw_code - F1;
        if (index >= 0 && index < NR_CONSOLES) { tty_select(index); }
    }

    //! TODO: other cases
}

//! read from the device to the rdbuf
//! TODO: support other devices except for console
static void tty_dev_read(tty_t *tty) {
    if (!vcon_is_foreground(tty->console)) { return; }
    keyboard_read(tty);
}

//! flush wrbuf to the device
static void tty_dev_write(tty_t *tty) {
    assert(tty != NULL);

    //! represent an invalid code
    const uint32_t NONE      = ~0;
    uint32_t       code      = NONE;
    uint32_t       prev_code = NONE;

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

        if (should_output) {
            char buf[1] = {code};
            vcon_write(tty->console, buf, 1);
        }
    }
}

void tty_wait_shell(int index) {
    int nr = index + 1;
    assert(tty_wait_nr == 0);
    tty_wait_nr = nr;
    while (tty_wait_nr == nr) { yield(); }
    kdebug("tty %d shell done", index);
}

int tty_wait_for() {
    int nr_tty = tty_wait_nr - 1;
    return nr_tty;
}

void tty_notify_shell() {
    tty_wait_nr = 0;
}

bool tty_select(int index) {
    if (index < 0 || index >= NR_CONSOLES) { return false; }
    bool should_setup_shell = false;
    if (tty_table[index] == NULL) {
        tty_t *tty = kmalloc(sizeof(tty_t));
        assert(tty != NULL);
        tty_init(tty);
        tty_table[index]   = tty;
        should_setup_shell = true;
    }
    tty_t *this     = tty_table[index];
    console_t *last = NULL;
    for (int i = 0; i < NR_CONSOLES; ++i) {
        tty_t *tty = tty_table[i];
        if (tty == NULL || tty == this) { continue; }
        if (vcon_is_foreground(tty->console)) {
            last = tty->console;
            break;
        }
    }
    //! NOTE: waiting for the initial shell is a time-consuming job, switch
    //! fg/bg after this might lead to a better interactive experience
    if (should_setup_shell) { tty_wait_shell(index); }
    if (last != NULL) { vcon_make_offscreen(last); }
    vcon_make_foreground(this->console);
    kdebug("make console %d foreground", index);
    return true;
}

void tty_handler() {
    tty_select(0);
    while (true) {
        int done = 0;
        for (int i = 0; i < NR_CONSOLES; ++i) {
            tty_t *tty = tty_table[i];
            if (tty == NULL) { continue; }
            int times = 0;
            tty_dev_read(tty);
            while (tty->cnt_wr > 0) {
                tty_dev_write(tty);
                tty_dev_read(tty);
                ++times;
            }
            if (times > 0) { ++done; }
        }
        if (done == 0) { yield(); }
    }
}
