/*
 *   The decision module for an automatic player for xtris.
 *
 *   Copyright (C) 1996 Roger Espel Llima <roger.espel.llima@pobox.com>
 *
 *   Started: 10 Oct 1996
 *   Version: 1.1
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation. See the file COPYING for details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decide.h"
#include "xtbot.h"

/*
 *  To make your own bots, you need to replace this module, defining
 *  the 2 functions:  decide() and init_decide().  They can
 *  use the external functions & variables explained in xtbot.h.
 *
 *  For a bot that uses a decision strategy based on evaluating the
 *  resulting pit after dropping the piece in each possible position
 *  and rotation, you can use the same decide() function given here,
 *  and replace only eval().  For more complex decision algorithms
 *  involving recursion and/or more state information, you'll need
 *  to adapt or rewrite decide() too.
 *
 *  See the file decide.h for a description of the parameters passed
 *  to decide().
 *
 */

/*  The default algorithm depends on a number of coefficients that
 *  define its priorities.
 */
static int coeff_f = 260, coeff_height = 110, coeff_hole = 450, coeff_y = 290;
static int coeff_pit = 190, coeff_ehole = 80;


/*  The old algorithm, basically a collection of ad-hoc tests... averages
 *  something like 500 lines on its own.
 */

#ifdef OLD_ALGORITHM

/* xx, yy, rotation  is the position where we're dropping the piece,
 * which is already in the pit[].  note that full lines have not been
 * dropped, so we need to do special tests to skip them.
 */
int eval(int xx, int yy, int rotation) {
  int x, y, p, v, i, j;
  int lines[20];

  for (i=0; i<20; i++) {
    lines[i] = 0;
    for (j=0; j<10; j++)
      if (pit[i][j] != EMPTY)
	lines[i]++;
  }

  v = 0;
  for (x=0; x<10; x++) {
    y = 0;
    while (y<20 && (pit[y][x] == EMPTY || lines[y] == 10))
      y++;
    y++;
    for (; y<20; y++)
      if (pit[y][x] == EMPTY)
	v -= 17;
    p = 0;
    y = 0;
    while (y<20 && (pit[y][x] == EMPTY || lines[y] == 10) && 
	   ((x > 0 && (pit[y][x-1] == EMPTY || lines[y] == 10)) || 
	    (x < 19 && (pit[y][x+1] == EMPTY || lines[y] == 10))))
      y++;
    for (; y<20 && pit[y][x] == EMPTY; y++, p++);
    if (p >= 2)
      v -= 5*(p-1);
  }
  i = 0;
  for (y=0; y<20; y++)
    if (lines[y] == 10)
      i++;
  switch(i) {
    case 0:
      break;
    case 1:
      v += 9;
      break;
    case 2:
      v += 50;
      break;
    case 3:
      v += 100;
      break;
    case 4:
      v += 200;
      break;
  }
  if (yy < 7)
    v -= 10;
  v += 3*yy + (xx > 2 ? xx - 2 : 2 - xx) - 2*(rotation&1);

  return v;
}

#else	/* OLD_ALGORITHM */

/*
 * The new algorithm computes 6 values on the whole pit, and returns
 * a weighted sum with them.  The values are:
 * . height = max height of the pieces in the pit
 * . holes = number of holes (empty positions with a full position somewhere
 *	     above them)
 * . frontier = length of the frontier between all full and empty zones
 *		(for each empty position, add 1 for each side of the position
 *		that touches a border or a full position).
 * . drop = how far down we're dropping the current brick
 * . pit = sum of the depths of all places where a long piece ( ====== )
 *	   would be needed.
 * . ehole = a sort of weighted sum of holes that attempts to calculate
 *	     how hard they are to fill.
 */

/* xx, yy, rotation  is the position where we're dropping the piece,
 * which is already in the pit[].  note that full lines have not been
 * dropped, so we need to do special tests to skip them.
 */
int eval(int xx, int yy, int rotation) {
  int i, ii, j, p, max_height, blocked, holes, e_holes, frontier, v;
  int lines[20];

#ifndef NO_EHOLES
  static int lin[20], hol[20][10], blockeds[10];
#endif

  v = 0;
  holes = 0;
  max_height = 20;
  frontier = 0;
  e_holes = 0;

  for (i=0; i<20; i++) {
    lines[i] = 0;
    for (j=0; j<10; j++)
      if (pit[i][j] != EMPTY)
	lines[i]++;
  }

#ifndef NO_EHOLES
  for (j=0; j<10; j++)
    blockeds[j] = -1;
  for (i=0; i<20; i++) {
    lin[i] = 0;
    for (j=0; j<10; j++) {
      if (pit[i][j] != EMPTY) {
	hol[i][j] = 0;
	blockeds[j] = i;
      } else {
	hol[i][j] = 1;
	if (blockeds[j] >= 0) {
	  ii = blockeds[j];
	  if (ii < i - 2)
	    ii = i - 2;
	  for (; ii<i; ii++)
	    if (pit[ii][j] != EMPTY)
	      hol[i][j] += lin[ii];
	}
	lin[i] += hol[i][j];
	e_holes += hol[i][j];
      }
    }
  }
#endif

  for (j=0; j<10; j++) {
    blocked = 0;
    for (i=0; i<20; i++) {
      if (lines[i] == 10)
	continue;
      if (pit[i][j] != EMPTY && i < max_height)
	max_height = i;
      if (pit[i][j] != EMPTY)
	blocked = 1;
      else {
        if (blocked)
	  holes++;
	if (i>0 && pit[i-1][j] != EMPTY)
	  frontier++;
	if (i<19 && pit[i+1][j] != EMPTY)
	  frontier++;
	if ((j>0 && pit[i][j-1] != EMPTY) || j==0)
	  frontier++;
	if ((j<9 && pit[i][j+1] != EMPTY) || j==9)
	  frontier++;
      }
    }
    p = i = 0;
    while (i<20 && (pit[i][j] == EMPTY || lines[i] == 10) && 
	   ((j > 0 && (pit[i][j-1] == EMPTY || lines[i] == 10)) || 
	    (j < 19 && (pit[i][j+1] == EMPTY || lines[i] == 10))))
      i++;
    for (; i<20 && pit[i][j] == EMPTY; i++, p++);
    if (p >= 2)
      v -= coeff_pit*(p-1);
  }

  return v - coeff_f*frontier - coeff_height * max_height - 
	  coeff_hole * holes + coeff_y*yy - coeff_ehole * e_holes;
}

#endif	/* OLD_ALGORITHM */

/* Your basic greedy decision algorithm: try all possibilities for the
 * current brick, evaluate each, return the one with the highest value
 * (randomizing if there are ties).
 */

void decide(int piece, int y0, int xmin, int xmax, int *newx, int *newy, 
	    int *newrot) {
  int rot = 0;
  int i, j, k, v, x, y;
  int maxval, ties;
  int values[10][4], ys[10][4];

  for (i=0; i<10; i++)
    for (j=0; j<4; j++)
      values[i][j] = -1000000000;
  maxval = -1000000000;
  ties = 0;

  for (x=xmin; x<=xmax; x++) {
    for (rot=0; rot<rotations[piece]; rot++) {
      y = y0;
      if (!fits(piece, rot, x, y))
	continue;
      while (fits(piece, rot, x, ++y));
      if (sticksout(piece, rot, x, --y))
	continue;
      put (piece, rot, x, y, CURRENT);

      values[x-xmin][rot] = v = eval(x, y, rot);
      ys[x-xmin][rot] = y;

      remove(piece, rot, x, y);

      if (v == maxval)
	ties++;
      else if (v > maxval) {
	maxval = v;
	ties = 1;
      }
    }
  }

  if (ties == 0) {
    *newx = *newy = *newrot = -1;
    return;
  }

  k = my_rand() % ties;

  for (x=xmin; x<=xmax; x++) {
    for (rot=0; rot<rotations[piece]; rot++) {
      if (values[x-xmin][rot] == maxval) {
	if (k == 0) {
	  *newy = ys[x-xmin][rot];
	  *newx = x;
	  *newrot = rot;
	  return;
	} else k--;
      }
    }
  }
  /* not reached */
  fatal("xtbot bug: impossible situation reached");
}

static void setvar(char *s, int *i) {
  char *t;
  if ((t = getenv(s)))
  *i = atoi(t);
}

/* Initialization: you can set pick the coefficients via environment
 * variables.
 */
void init_decide(void) {
  setvar("XTBOT_FRONTIER", &coeff_f);
  setvar("XTBOT_HEIGHT", &coeff_height);
  setvar("XTBOT_HOLE", &coeff_hole);
  setvar("XTBOT_DROP", &coeff_y);
  setvar("XTBOT_PIT", &coeff_pit);
  setvar("XTBOT_EHOLE", &coeff_ehole);
}

