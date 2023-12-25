#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "keyboard.h"
#include "keymap.h"
#include "x86.h"


static KB_INPUT kb_in;
static MOUSE_INPUT mouse_in;
static int mouse_init;

static	int		code_with_E0;
static	int		shift_l;	/* l shift state	*/
static	int		shift_r;	/* r shift state	*/
static	int		alt_l;		/* l alt state		*/
static	int		alt_r;		/* r left state		*/
static	int		ctrl_l;		/* l ctrl state		*/
static	int		ctrl_r;		/* l ctrl state		*/
static	int		caps_lock;	/* Caps Lock		*/
static	int		num_lock;	/* Num Lock		*/
static	int		scroll_lock;	/* Scroll Lock		*/
static	int		column;

static u8		get_byte_from_kb_buf();
static void	set_leds();
static void	set_mouse_leds();
static void	kb_wait();
// static void	kb_ack();


void kb_handler(int irq){
	u8 scan_code =  inb(0x60);
	if(kb_in.count < KB_IN_BYTES){
		*(kb_in.p_head) = scan_code;
		kb_in.p_head++;
		if(kb_in.p_head==kb_in.buf+KB_IN_BYTES){
			kb_in.p_head = kb_in.buf;
		}
		kb_in.count++;
	}

};

#define TTY_FIRST (tty_table)
#define TTY_END (tty_table+NR_CONSOLES)

void mouse_handler(int irq){
	u8 scan_code =  inb(0x60);
	if(!mouse_init){
		mouse_init = 1;
		return;
	}
	mouse_in.buf[mouse_in.count]=scan_code;
	mouse_in.count++;
	if(mouse_in.count==3){
		TTY* p_tty;
		for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
			if(p_tty->console==&console_table[current_console]){
				p_tty->mouse_left_button = mouse_in.buf[0]&0x01;
				
				u8 mid_button = mouse_in.buf[0]&0b100;
				if(mid_button==0b100){
					p_tty->mouse_mid_button = 1;
				}else{
					p_tty->mouse_mid_button = 0;
				}

				if(p_tty->mouse_left_button){
					u8 dir_Y = mouse_in.buf[0]&0x20;
					u8 dir_X = mouse_in.buf[0]&0x10;
					if(dir_Y==0x20){//down
						p_tty->mouse_Y -= 1;
					}else{//up
						p_tty->mouse_Y += 1;
					}

					if(dir_X==0x10){//left
						p_tty->mouse_X -= 1;
					}else{//right
						p_tty->mouse_X += 1;
					}
				}
			}
		}
		
		mouse_in.count=0;
	}


}

void init_mouse(){
	mouse_in.count = 0;
	
	put_irq_handler(MOUSE_IRQ,mouse_handler);
	enable_irq(MOUSE_IRQ);

}

void init_kb(){
	kb_in.count = 0;
	kb_in.p_head = kb_in.p_tail = kb_in.buf;

	shift_l	= shift_r = 0;
	alt_l	= alt_r   = 0;
	ctrl_l	= ctrl_r  = 0;

	caps_lock	= 0;
	num_lock	= 1;
	scroll_lock	= 0;

	column		= 0;
	
	set_leds();
	put_irq_handler(KEYBOARD_IRQ, kb_handler);
	enable_irq(KEYBOARD_IRQ);

	init_mouse();
	set_mouse_leds();

}

void keyboard_read(TTY* p_tty)
{
	u8	scan_code;

	/**
	 * 1 : make
	 * 0 : break
	 */
	int	make;

	/**
	 * We use a integer to record a key press.
	 * For instance, if the key HOME is pressed, key will be evaluated to
	 * `HOME' defined in keyboard.h.
	 */
	u32	key = 0;


	/**
	 * This var points to a row in keymap[]. I don't use two-dimension
	 * array because I don't like it.
	 */
	u32*	keyrow;

	while (kb_in.count > 0) {
		code_with_E0 = 0;
		scan_code = get_byte_from_kb_buf();

		/* parse the scan code below */
		if (scan_code == 0xE1) {
			int i;
			u8 pausebreak_scan_code[] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5};
			int is_pausebreak = 1;
			for (i = 1; i < 6; i++) {
				if (get_byte_from_kb_buf() != pausebreak_scan_code[i]) {
					is_pausebreak = 0;
					break;
				}
			}
			if (is_pausebreak) {
				key = PAUSEBREAK;
			}
		}
		else if (scan_code == 0xE0) {
			code_with_E0 = 1;
			scan_code = get_byte_from_kb_buf();

			/* PrintScreen is pressed */
			if (scan_code == 0x2A) {
				code_with_E0 = 0;
				if ((scan_code = get_byte_from_kb_buf()) == 0xE0) {
					code_with_E0 = 1;
					if ((scan_code = get_byte_from_kb_buf()) == 0x37) {
						key = PRINTSCREEN;
						make = 1;
					}
				}
			}
			/* PrintScreen is released */
			else if (scan_code == 0xB7) {
				code_with_E0 = 0;
				if ((scan_code = get_byte_from_kb_buf()) == 0xE0) {
					code_with_E0 = 1;
					if ((scan_code = get_byte_from_kb_buf()) == 0xAA) {
						key = PRINTSCREEN;
						make = 0;
					}
				}
			}
		}

		if ((key != PAUSEBREAK) && (key != PRINTSCREEN)) {
			int caps;

			/* make or break */
			make = (scan_code & FLAG_BREAK ? 0 : 1);
			
			keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];

			column = 0;

			caps = shift_l || shift_r;
			if (caps_lock &&
			    keyrow[0] >= 'a' && keyrow[0] <= 'z')
				caps = !caps;

			if (caps)
				column = 1;

			if (code_with_E0)
				column = 2;

			key = keyrow[column];

			switch(key) {
			case SHIFT_L:
				shift_l	= make;
				break;
			case SHIFT_R:
				shift_r	= make;
				break;
			case CTRL_L:
				ctrl_l	= make;
				break;
			case CTRL_R:
				ctrl_r	= make;
				break;
			case ALT_L:
				alt_l	= make;
				break;
			case ALT_R:
				alt_l	= make;
				break;
			case CAPS_LOCK:
				if (make) {
					caps_lock   = !caps_lock;
					set_leds();
				}
				break;
			case NUM_LOCK:
				if (make) {
					num_lock    = !num_lock;
					set_leds();
				}
				break;
			case SCROLL_LOCK:
				if (make) {
					scroll_lock = !scroll_lock;
					set_leds();
				}
				break;
			default:
				break;
			}
		}

		if(make){ /* Break Code is ignored */
			int pad = 0;

			/* deal with the numpad first */
			if ((key >= PAD_SLASH) && (key <= PAD_9)) {
				pad = 1;
				switch(key) {	/* '/', '*', '-', '+',
						 * and 'Enter' in num pad
						 */
				case PAD_SLASH:
					key = '/';
					break;
				case PAD_STAR:
					key = '*';
					break;
				case PAD_MINUS:
					key = '-';
					break;
				case PAD_PLUS:
					key = '+';
					break;
				case PAD_ENTER:
					key = ENTER;
					break;
				default:
					/* the value of these keys
					 * depends on the Numlock
					 */
					if (num_lock) {	/* '0' ~ '9' and '.' in num pad */
						if (key >= PAD_0 && key <= PAD_9)
							key = key - PAD_0 + '0';
						else if (key == PAD_DOT)
							key = '.';
					}
					else{
						switch(key) {
						case PAD_HOME:
							key = HOME;
							break;
						case PAD_END:
							key = END;
							break;
						case PAD_PAGEUP:
							key = PAGEUP;
							break;
						case PAD_PAGEDOWN:
							key = PAGEDOWN;
							break;
						case PAD_INS:
							key = INSERT;
							break;
						case PAD_UP:
							key = UP;
							break;
						case PAD_DOWN:
							key = DOWN;
							break;
						case PAD_LEFT:
							key = LEFT;
							break;
						case PAD_RIGHT:
							key = RIGHT;
							break;
						case PAD_DOT:
							key = DELETE;
							break;
						default:
							break;
						}
					}
					break;
				}
			}
			key |= shift_l	? FLAG_SHIFT_L	: 0;
			key |= shift_r	? FLAG_SHIFT_R	: 0;
			key |= ctrl_l	? FLAG_CTRL_L	: 0;
			key |= ctrl_r	? FLAG_CTRL_R	: 0;
			key |= alt_l	? FLAG_ALT_L	: 0;
			key |= alt_r	? FLAG_ALT_R	: 0;
			key |= pad	? FLAG_PAD	: 0;

			in_process(p_tty,key);
		}
	}
}



/*****************************************************************************
 *                                get_byte_from_kb_buf
 *****************************************************************************/
/**
 * Read a byte from the keyboard buffer.
 * 
 * @return The byte read.
 *****************************************************************************/
static u8 get_byte_from_kb_buf()
{
	u8	scan_code;

	while (kb_in.count <= 0) {} /* wait for a byte to arrive */

	disable_int();		/* for synchronization */
	scan_code = *(kb_in.p_tail);
	kb_in.p_tail++;
	if (kb_in.p_tail == kb_in.buf + KB_IN_BYTES) {
		kb_in.p_tail = kb_in.buf;
	}
	kb_in.count--;
	enable_int();		/* for synchronization */

	return scan_code;
}


/*****************************************************************************
 *                                kb_wait
 *****************************************************************************/
/**
 * Wait until the input buffer of 8042 is empty.
 * 
 *****************************************************************************/
static void kb_wait()	/* 等待 8042 的输入缓冲区空 */
{
	u8 kb_stat;

	do {
		kb_stat = inb(KB_CMD);
		
	} while (kb_stat & 0x02);
}


/*****************************************************************************
 *                                kb_ack
 *****************************************************************************/
/**
 * Read from the keyboard controller until a KB_ACK is received.
 * 
 *****************************************************************************/
// static void kb_ack()
// {
// 	u8 kb_read;

// 	do {
// 		kb_read = inb(KB_DATA);
// 	} while (kb_read != KB_ACK);
// }


/*****************************************************************************
 *                                set_leds
 *****************************************************************************/
/**
 * Set the leds according to: caps_lock, num_lock & scroll_lock.
 * 
 *****************************************************************************/
static void set_leds()
{
	kb_wait();
	outb(KB_CMD, KEYCMD_WRITE_MODE);
	kb_wait();
	outb(KB_DATA, KBC_MODE);
}

static void set_mouse_leds(){
	kb_wait();
	outb(KB_CMD,KBCMD_EN_MOUSE_INTFACE);
	kb_wait();
	outb(KB_CMD, KEYCMD_SENDTO_MOUSE);
	kb_wait();
	outb(KB_DATA, MOUSECMD_ENABLE);
	kb_wait();
	outb(KB_CMD, KEYCMD_WRITE_MODE);
	kb_wait();
	outb(KB_DATA, KBC_MODE);
}

