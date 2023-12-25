/*************************************************************************//**
 *****************************************************************************
 * @file   tty.h
 * @brief  
 * @author Forrest Y. Yu
 * @date   2005
 *****************************************************************************
 *****************************************************************************/

/**********************************************************
*	tty.h       //added by mingxuan 2019-5-17
***********************************************************/

#ifndef	_ORANGES_TTY_H_
#define	_ORANGES_TTY_H_
#include "type.h"

#define TTY_IN_BYTES		256	/* tty input queue size */
#define TTY_OUT_BUF_LEN		2	/* tty output buffer size */

/*  TTY state (3bit)
	wait_enter  wait_space  display
	    1/0        1/0        1/0
*/

#define TTY_STATE_WAIT_ENTER  4  /*100*/
#define TTY_STATE_WAIT_SPACE  2  /*010*/
#define TTY_STATE_DISPLAY     1  /*001*/

struct s_tty;
struct s_console;

/* TTY */
typedef struct s_tty
{
	u32	ibuf[TTY_IN_BYTES];	/* TTY input buffer */
	u32*	ibuf_head;		/* the next free slot */
	u32*	ibuf_tail;		/* 缓冲区显示位置指针 */
	u32*	ibuf_read;		
	int	ibuf_cnt;		/* how many */
	int ibuf_read_cnt;
	int status;

	int mouse_left_button;
	int mouse_mid_button;
	int mouse_X;
	int mouse_Y;

	struct s_console *	console;
}TTY;

#include "console.h"

void select_console(int nr_console);
void init_screen(TTY* tty);
void out_char(CONSOLE* con, char ch);
int is_current_console(CONSOLE* con);

#endif /* _ORANGES_TTY_H_ */