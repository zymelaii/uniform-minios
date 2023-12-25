/*************************************************************************//**
 *****************************************************************************
 * @file   console.h
 * @brief  
 * @author Forrest Y. Yu
 * @date   2005
 *****************************************************************************
 *****************************************************************************/

/**********************************************************
*	console.h       //added by mingxuan 2019-5-17
***********************************************************/

#ifndef	_ORANGES_CONSOLE_H_
#define	_ORANGES_CONSOLE_H_
#include "const.h"

/* CONSOLE */
typedef struct s_console
{
	unsigned int	crtc_start; /* set CRTC start addr reg */
	unsigned int	orig;	    /* start addr of the console */
	unsigned int	con_size;   /* how many words does the console have */
	unsigned int	cursor;
	int		is_full;
	unsigned int 	current_line;
}CONSOLE;


#define SCR_UP	1	/* scroll upward */
#define SCR_DN	-1	/* scroll downward */

#define SCR_WIDTH 80
#define SCR_HEIGHT 25
#define SCR_SIZE (SCR_HEIGHT * SCR_WIDTH)
#define SCR_BUFSIZE (2 * SCR_SIZE)
#define SCR_MAXLINE (SCR_BUFSIZE / SCR_WIDTH)

#define FLASH_CHAR 0x8000
#define DEFAULT_CHAR_COLOR ((MAKE_COLOR(BLACK, WHITE | BRIGHT)) << 8)
#define GRAY_CHAR (MAKE_COLOR(BLACK, BLACK) | BRIGHT)
#define RED_CHAR (MAKE_COLOR(BLUE, RED) | BRIGHT)
#define WHITE_CHAR (MAKE_COLOR(BLACK, WHITE | BRIGHT))
#define MAKE_CELL(clr, ch) (clr | ch)
#define BLANK MAKE_CELL(DEFAULT_CHAR_COLOR, ' ')

#endif /* _ORANGES_CONSOLE_H_ */