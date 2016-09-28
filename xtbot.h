/*
 *  Interface provided by the xtbot skeleton to the decision module
 *
 *  Copyright (C) 1996 Roger Espel Llima <roger.espel.llima@pobox.com>
 *
 *   Ported 2004-05-24 to win32 platform using Eclipse (+CDT) platform with
 *   Borland BCC55 free command line tools and lcc-win32 system by
 *   Vedran Vidovic <vvidovic@inet.hr>.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation. See the file COPYING for details.
 *
 */

/* These are the external variables and functions that the decision module
 * can use:
 */

/* piece shapes, as defined in xtbot.c;  see the functions put() and fits()
 * for the encoding...  the evaluation routines shouldn't need to use the
 * shapes directly anyway.
 */
extern int shape[7][4][8];

/* for each of the pieces, how many of their rotations are actually different.
 * used to check all combinations w/o checking each position more than once
 * for pieces like the square.
 */
extern int rotations[7];

/* the tetris pit;  positions are numbered from 0,0 (top left) to
 * 19,9 (bottom right).  note that the row number is the FIRST index,
 * not the second.
 */
extern int pit[20][10];

/* exits with an error message;  use for cases like malloc() failing
 * or some impossible condition being reached
 */
extern void fatal(char *);

/* a pretty good random number generator.
 */
extern unsigned int my_rand(void);

/* check if a piece fits at a given position in the pit;  the arguments
 * are the piece number, the rotation number (0 to 3), the column and
 * the row.  note that this will return true even if the piece sticks out
 * at the top of the screen, which is valid for pieces that are still falling
 * if they get rotated while they're at the top, but not for pieces that
 * are done falling.
 */
extern int fits (int piece, int rot, int x, int y);

/* check if a piece sticks out at the top of the screen.
 */
extern int sticksout(int piece, int rot, int x, int y);

/* put a piece at a given position on the screen.  'type' usually
 * the same as 'piece', each type is given a different color on the
 * screen.  the special value EMPTY means that the position is empty,
 * and the special value CURRENT indicates the piece that is currently
 * being tested at some position for evaluation (and that is therefore
 * not going to be drawn with that type.
 */
extern void put(int piece, int rot, int x, int y, int type);

#define remove(p, r, x, y) put(p, r, x, y, EMPTY)

#define EMPTY 7
#define GRAY 8
#define CURRENT 255

