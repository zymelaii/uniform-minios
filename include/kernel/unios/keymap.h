#pragma once

#include <unios/keyboard.h>
#include <stdint.h>

typedef uint32_t keymap_entry_t[MAP_COLS];

//! Keymap for US MF-2 keyboard.
extern keymap_entry_t keymap[NR_SCAN_CODES];

/*
======================================================================
                        Appendix: Scan code set 1
======================================================================
KEY     MAKE    BREAK|  KEY     MAKE    BREAK | KEY     MAKE    BREAK
----------------------------------------------------------------------
A       1E      9E   |  9       0A      8A    | [       1A      9A
B       30      B0   |  `       29      89    | INSERT  E0,52   E0,D2
C       2E      AE   |  -       0C      8C    | HOME    E0,47   E0,C7
D       20      A0   |  =       0D      8D    | PG UP   E0,49   E0,C9
E       12      92   |  \       2B      AB    | DELETE  E0,53   E0,D3
F       21      A1   |  BKSP    0E      8E    | END     E0,4F   E0,CF
G       22      A2   |  SPACE   39      B9    | PG DN   E0,51   E0,D1
H       23      A3   |  TAB     0F      8F    | U ARROW E0,48   E0,C8
I       17      97   |  CAPS    3A      BA    | L ARROW E0,4B   E0,CB
J       24      A4   |  L SHFT  2A      AA    | D ARROW E0,50   E0,D0
K       25      A5   |  L CTRL  1D      9D    | R ARROW E0,4D   E0,CD
L       26      A6   |  L GUI   E0,5B   E0,DB | NUM     45      C5
M       32      B2   |  L ALT   38      B8    | KP /    E0,35   E0,B5
N       31      B1   |  R SHFT  36      B6    | KP *    37      B7
O       18      98   |  R CTRL  E0,1D   E0,9D | KP -    4A      CA
P       19      99   |  R GUI   E0,5C   E0,DC | KP +    4E      CE
Q       10      19   |  R ALT   E0,38   E0,B8 | KP EN   E0,1C   E0,9C
R       13      93   |  APPS    E0,5D   E0,DD | KP .    53      D3
S       1F      9F   |  ENTER   1C      9C    | KP 0    52      D2
T       14      94   |  ESC     01      81    | KP 1    4F      CF
U       16      96   |  F1      3B      BB    | KP 2    50      D0
V       2F      AF   |  F2      3C      BC    | KP 3    51      D1
W       11      91   |  F3      3D      BD    | KP 4    4B      CB
X       2D      AD   |  F4      3E      BE    | KP 5    4C      CC
Y       15      95   |  F5      3F      BF    | KP 6    4D      CD
Z       2C      AC   |  F6      40      C0    | KP 7    47      C7
0       0B      8B   |  F7      41      C1    | KP 8    48      C8
1       02      82   |  F8      42      C2    | KP 9    49      C9
2       03      83   |  F9      43      C3    | ]       1B      9B
3       04      84   |  F10     44      C4    | ;       27      A7
4       05      85   |  F11     57      D7    | '       28      A8
5       06      86   |  F12     58      D8    | ,       33      B3
                     |                        |
6       07      87   |  PRTSCRN E0,2A   E0,B7 | .       34      B4
                     |          E0,37   E0,AA |
                     |                        |
7       08      88   |  SCROLL  46      C6    | /       35      B5
                     |                        |
8       09      89   |  PAUSE   E1,1D         |
                     |          45,E1,  -NONE-|
                     |          9D,C5         |
----------------------------------------------------------------------
ACPI Scan Codes:
----------------------------------------------------------------------
Key             Make Code       Break Code
----------------------------------------------------------------------
Power           E0, 5E          E0, DE
Sleep           E0, 5F          E0, DF
Wake            E0, 63          E0, E3
----------------------------------------------------------------------
Windows Multimedia Scan Codes:
----------------------------------------------------------------------
Key             Make Code       Break Code
----------------------------------------------------------------------
Next Track      E0, 19          E0, 99
Previous Track  E0, 10          E0, 90
Stop            E0, 24          E0, A4
Play/Pause      E0, 22          E0, A2
Mute            E0, 20          E0, A0
Volume Up       E0, 30          E0, B0
Volume Down     E0, 2E          E0, AE
Media Select    E0, 6D          E0, ED
E-Mail          E0, 6C          E0, EC
Calculator      E0, 21          E0, A1
My Computer     E0, 6B          E0, EB
WWW Search      E0, 65          E0, E5
WWW Home        E0, 32          E0, B2
WWW Back        E0, 6A          E0, EA
WWW Forward     E0, 69          E0, E9
WWW Stop        E0, 68          E0, E8
WWW Refresh     E0, 67          E0, E7
WWW Favorites   E0, 66          E0, E6
----------------------------------------------------------------------
*/
