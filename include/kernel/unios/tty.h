#pragma once

#include <stdint.h>
#include "console.h"

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

    struct s_console* console;
} tty_t;

void select_console(int nr_console);
void init_screen(tty_t* tty);
void out_char(CONSOLE* con, char ch);
int  is_current_console(CONSOLE* con);
