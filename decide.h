/*
 *  Functions provided by the evaluation module.
 *
 *  Copyright (C) 1996 Roger Espel Llima <roger.espel.llima@pobox.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation. See the file COPYING for details.
 *
 */

/* The decision module defines these functions, used by xtbot */


/* This function decides, on a given pit, where to place the piece 'piece'.
 * The pit does not contain the piece when decide() is called, and must not
 * contain it on return.
 *
 * y0 is the 'y' position of the piece at the top of the screen, and
 * xmin .. xmax is the range of values for x, to pass them as arguments
 * to functions like put() and fits(). 
 * 
 * the function returns the chosen position for the piece in the variables
 * newx (column number), newy (row number after dropping it as far as it
 * will go), and newrot (rotation number, from 0 to 3).
 */
extern void decide (int piece, int y0, int xmin, int xmax, int *newx, 
			int *newy, int *newrot);

/* This function gets called once at startup.  Define it to do nothing
 * if your decision doesn't require any initialization.
 */
extern void init_decide(void);


