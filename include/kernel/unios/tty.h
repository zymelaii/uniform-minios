#pragma once

#include <unios/console.h>
#include <stdint.h>

#define TTY_BUFSZ 256 //<! tty buffer size

enum tty_status {
    TTY_DISPLAY    = 1,
    TTY_WAIT_SPACE = 2,
    TTY_WAIT_ENTER = 4,
};

enum mouse_buttons {
    MOUSE_LEFT_BUTTON   = 1,
    MOUSE_RIGHT_BUTTON  = 2,
    MOUSE_MIDDLE_BUTTON = 4,
};

typedef struct tty_s {
    u32  ibuf[TTY_BUFSZ]; //<! tty buffer, shared by in/out
    u32* ibuf_next;       //<! next free buffer pos

    //! NOTE: there's always cnt_wr <= cnt_rd
    u32* ibuf_wr; //<! current wrbuf pos
    u32* ibuf_rd; //<! current rdbuf pos
    int  cnt_wr;  //<! wrbuf size
    int  cnt_rd;  //<! rdbuf size

    int status;

    struct {
        int buttons; //<! button down flags
        int off_x;   //<! offset x till last update
        int off_y;   //<! offset y till last update
    } mouse;

    console_t* console;
} tty_t;

void select_console(int nr_console);
void init_screen(tty_t* tty);
int  is_current_console(console_t* con);

void tty_write(tty_t* tty, char* buf, int len);
int  tty_read(tty_t* tty, char* buf, int len);
void tty_keyboard_proc(tty_t* tty, u32 key);

void tty_handler();

extern tty_t tty_table[];
