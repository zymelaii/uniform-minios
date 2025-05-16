#include <unios/keyboard.h>
#include <unios/interrupt.h>
#include <unios/keymap.h>
#include <unios/schedule.h>
#include <arch/x86.h>
#include <sys/defs.h>
#include <atomic.h>

static KB_INPUT    kb_in;
static MOUSE_INPUT mouse_in;
static int         mouse_init;

static int shift_l;     //<! left shift state
static int shift_r;     //<! right shift state
static int alt_l;       //<! left alt state
static int alt_r;       //<! right left state
static int ctrl_l;      //<! left ctrl state
static int ctrl_r;      //<! left ctrl state
static int caps_lock;   //<! Caps Lock
static int num_lock;    //<! Num Lock
static int scroll_lock; //<! Scroll Lock

/*!
 * \brief wait until the input buffer of 8042 is empty.
 */
static void kb_wait() {
    uint8_t kb_stat;
    do { kb_stat = inb(KB_CMD); } while (kb_stat & 0x02);
}

/*!
 * \brief set the leds according to: caps_lock, num_lock & scroll_lock.
 */
static void set_leds() {
    kb_wait();
    outb(KB_CMD, KEYCMD_WRITE_MODE);
    kb_wait();
    outb(KB_DATA, KBC_MODE);
}

static void set_mouse_leds() {
    kb_wait();
    outb(KB_CMD, KBCMD_EN_MOUSE_INTFACE);
    kb_wait();
    outb(KB_CMD, KEYCMD_SENDTO_MOUSE);
    kb_wait();
    outb(KB_DATA, MOUSECMD_ENABLE);
    kb_wait();
    outb(KB_CMD, KEYCMD_WRITE_MODE);
    kb_wait();
    outb(KB_DATA, KBC_MODE);
}

/*!
 * \brief read a byte from the keyboard buffer.
 */
static uint8_t get_byte_from_kb_buf() {
    uint8_t scan_code;

    //! wait for a byte to arrive
    while (kb_in.count <= 0) {}

    disable_int_begin();
    scan_code = *(kb_in.p_tail);
    kb_in.p_tail++;
    if (kb_in.p_tail == kb_in.buf + KB_IN_BYTES) { kb_in.p_tail = kb_in.buf; }
    kb_in.count--;
    disable_int_end();

    return scan_code;
}

void kb_handler(int irq) {
    uint8_t scan_code = inb(0x60);
    if (kb_in.count < KB_IN_BYTES) {
        *(kb_in.p_head) = scan_code;
        kb_in.p_head++;
        if (kb_in.p_head == kb_in.buf + KB_IN_BYTES) {
            kb_in.p_head = kb_in.buf;
        }
        kb_in.count++;
    }
};

void mouse_handler(int irq) {
    lock_or(&mouse_in.lock, schedule);
    uint8_t scan_code = inb(0x60);
    if (!mouse_init) {
        release(&mouse_in.lock);
        mouse_init = 1;
        return;
    }

    mouse_in.buf[mouse_in.count++] = scan_code;

    if (mouse_in.count != 3) {
        release(&mouse_in.lock);
        return;
    }

    tty_t* tty = NULL;
    for (int i = 0; i < NR_CONSOLES; ++i) {
        tty_t* this = tty_table[i];
        if (this == NULL) { continue; }
        if (!vcon_is_foreground(this->console)) {
            tty = this;
            break;
        }
    }

    do {
        if (tty == NULL) { break; }

        if (mouse_in.buf[0] & 0b001) {
            tty->mouse.buttons |= MOUSE_LEFT_BUTTON;
        } else {
            tty->mouse.buttons &= ~MOUSE_LEFT_BUTTON;
        }

        if (mouse_in.buf[0] & 0b010) {
            tty->mouse.buttons |= MOUSE_RIGHT_BUTTON;
        } else {
            tty->mouse.buttons &= ~MOUSE_RIGHT_BUTTON;
        }

        if (mouse_in.buf[0] & 0b100) {
            tty->mouse.buttons |= MOUSE_MIDDLE_BUTTON;
        } else {
            tty->mouse.buttons &= ~MOUSE_MIDDLE_BUTTON;
        }

        //! drag
        if (tty->mouse.buttons & MOUSE_LEFT_BUTTON) {
            uint8_t x_dir     = mouse_in.buf[0] & 0x10;
            uint8_t y_dir     = mouse_in.buf[0] & 0x20;
            tty->mouse.off_x += x_dir == 0x10 ? -1 : +1;
            tty->mouse.off_y += y_dir == 0x20 ? -1 : +1;
        }
    } while (0);

    mouse_in.count = 0;
    release(&mouse_in.lock);
}

void init_mouse() {
    mouse_in.count = 0;
    mouse_in.lock  = 0;

    put_irq_handler(MOUSE_IRQ, mouse_handler);
    enable_irq(MOUSE_IRQ);
}

void init_keyboard() {
    kb_in.count  = 0;
    kb_in.p_head = kb_in.p_tail = kb_in.buf;

    shift_l = shift_r = 0;
    alt_l = alt_r = 0;
    ctrl_l = ctrl_r = 0;

    caps_lock   = 0;
    num_lock    = 1;
    scroll_lock = 0;

    set_leds();
    put_irq_handler(KEYBOARD_IRQ, kb_handler);
    enable_irq(KEYBOARD_IRQ);

    init_mouse();
    set_mouse_leds();
}

void keyboard_read(tty_t* p_tty) {
    //! make or break
    bool make = false;

    //! we use a integer to record a key press. for instance, if the key home is
    //! pressed, key will be evaluated to `HOME' defined in keyboard.h.
    uint32_t key = 0;

    while (kb_in.count > 0) {
        bool    code_with_E0 = false;
        uint8_t scan_code    = get_byte_from_kb_buf();

        if (scan_code == 0xe1) {
            uint8_t pausebreak_scan_code[] = {
                0xe1, 0x1d, 0x45, 0xe1, 0x9d, 0xc5};
            int is_pausebreak = 1;
            for (int i = 1; i < 6; ++i) {
                if (get_byte_from_kb_buf() != pausebreak_scan_code[i]) {
                    is_pausebreak = 0;
                    break;
                }
            }
            if (is_pausebreak) { key = PAUSEBREAK; }
        } else if (scan_code == 0xe0) {
            code_with_E0 = true;
            scan_code    = get_byte_from_kb_buf();
            if (scan_code == 0x2a) {
                //! PrintScreen pressed
                code_with_E0 = false;
                if ((scan_code = get_byte_from_kb_buf()) == 0xe0) {
                    code_with_E0 = true;
                    if ((scan_code = get_byte_from_kb_buf()) == 0x37) {
                        key  = PRINTSCREEN;
                        make = true;
                    }
                }
            } else if (scan_code == 0xb7) {
                //! PrintScreen release
                code_with_E0 = false;
                if ((scan_code = get_byte_from_kb_buf()) == 0xe0) {
                    code_with_E0 = true;
                    if ((scan_code = get_byte_from_kb_buf()) == 0xaa) {
                        key  = PRINTSCREEN;
                        make = false;
                    }
                }
            }
        }

        if ((key != PAUSEBREAK) && (key != PRINTSCREEN)) {
            uint32_t* keyrow = keymap[scan_code & 0x7f];
            make             = !(scan_code & FLAG_BREAK);
            int column       = 0;

            bool caps = shift_l || shift_r;
            if (caps_lock && keyrow[0] >= 'a' && keyrow[0] <= 'z') {
                caps = !caps;
            }

            if (caps) { column = 1; }
            if (code_with_E0) { column = 2; }

            key = keyrow[column];

            switch (key) {
                case SHIFT_L: {
                    shift_l = make;
                } break;
                case SHIFT_R: {
                    shift_r = make;
                } break;
                case CTRL_L: {
                    ctrl_l = make;
                } break;
                case CTRL_R: {
                    ctrl_r = make;
                } break;
                case ALT_L: {
                    alt_l = make;
                } break;
                case ALT_R: {
                    alt_l = make;
                } break;
                case CAPS_LOCK: {
                    if (make) {
                        caps_lock = !caps_lock;
                        set_leds();
                    }
                } break;
                case NUM_LOCK: {
                    if (make) {
                        num_lock = !num_lock;
                        set_leds();
                    }
                } break;
                case SCROLL_LOCK: {
                    if (make) {
                        scroll_lock = !scroll_lock;
                        set_leds();
                    }
                } break;
            }
        }

        //! break code is ignored
        if (!make) { continue; }

        //! deal with the numpad first
        bool numpad = key >= PAD_SLASH && key <= PAD_9;
        if (numpad) {
            switch (key) {
                case PAD_SLASH: {
                    key = '/';
                } break;
                case PAD_STAR: {
                    key = '*';
                } break;
                case PAD_MINUS: {
                    key = '-';
                } break;
                case PAD_PLUS: {
                    key = '+';
                } break;
                case PAD_ENTER: {
                    key = ENTER;
                } break;
                default: {
                    //! value of these keys depends on the Numlock
                    if (num_lock) {
                        //! '0'~'9' & '.'
                        if (key >= PAD_0 && key <= PAD_9) {
                            key = key - PAD_0 + '0';
                        } else if (key == PAD_DOT) {
                            key = '.';
                        }
                        break;
                    }
                    switch (key) {
                        case PAD_HOME: {
                            key = HOME;
                        } break;
                        case PAD_END: {
                            key = END;
                        } break;
                        case PAD_PAGEUP: {
                            key = PAGEUP;
                        } break;
                        case PAD_PAGEDOWN: {
                            key = PAGEDOWN;
                        } break;
                        case PAD_INS: {
                            key = INSERT;
                        } break;
                        case PAD_UP: {
                            key = UP;
                        } break;
                        case PAD_DOWN: {
                            key = DOWN;
                        } break;
                        case PAD_LEFT: {
                            key = LEFT;
                        } break;
                        case PAD_RIGHT: {
                            key = RIGHT;
                        } break;
                        case PAD_DOT: {
                            key = DELETE;
                        } break;
                    }
                } break;
            }
        }

        key |= shift_l ? FLAG_SHIFT_L : 0;
        key |= shift_r ? FLAG_SHIFT_R : 0;
        key |= ctrl_l ? FLAG_CTRL_L : 0;
        key |= ctrl_r ? FLAG_CTRL_R : 0;
        key |= alt_l ? FLAG_ALT_L : 0;
        key |= alt_r ? FLAG_ALT_R : 0;
        key |= numpad ? FLAG_PAD : 0;

        tty_keyboard_proc(p_tty, key);
    }
}
