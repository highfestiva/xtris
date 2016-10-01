/*
 *   A multi-player version of Tetris for the X Window System
 *
 *   Copyright (C) 1996 Roger Espel Llima <roger.espel.llima@pobox.com>
 *
 *   Started: 10 Oct 1996
 *   Version: 1.15
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

#define VERSION "1.18 (Win)"

#define _WIN32_WINNT	0x0600	// Vista+ for inet_pton.
#pragma comment(lib, "Ws2_32.lib")

#include <WinSock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <process.h>
#include "systime.h"

#define DEFAULTPORT 19503

#define PROT_VERS_1 1
#define PROT_VERS_2 0
#define PROT_VERS_3 1

#define OP_NICK 1
#define OP_PLAY 2
#define OP_FALL 3
#define OP_DRAW 4
#define OP_LOST 5
#define OP_GONE 6
#define OP_CLEAR 7
#define OP_NEW 8
#define OP_LINES 9
#define OP_GROW 10
#define OP_MODE 11
#define OP_LEVEL 12
#define OP_BOT 13
#define OP_KILL 14
#define OP_PAUSE 15
#define OP_CONT 16
#define OP_VERSION 17
#define OP_BADVERS 18
#define OP_MSG 19
#define OP_YOUARE 20
#define OP_LINESTO 21
#define OP_WON 22
#define OP_ZERO 23


static int shape[7][4][8] = {

    /*  The I   ++++++++  */
 {
    {  1, 3, 2, 3, 3, 3, 4, 3 },
    {  3, 1, 3, 2, 3, 3, 3, 4 },
    {  1, 3, 2, 3, 3, 3, 4, 3 },
    {  3, 1, 3, 2, 3, 3, 3, 4 }
 },
    /*  The O      ++++
                   ++++   */
 {
    {  2, 3, 3, 3, 2, 2, 3, 2 },
    {  2, 3, 3, 3, 2, 2, 3, 2 },
    {  2, 3, 3, 3, 2, 2, 3, 2 },
    {  2, 3, 3, 3, 2, 2, 3, 2 }
 },

    /*  The T   ++++++
                  ++      */
 {
    {  3, 2, 2, 3, 3, 3, 4, 3 },
    {  3, 2, 3, 3, 3, 4, 4, 3 },
    {  2, 2, 3, 2, 4, 2, 3, 3 },
    {  2, 3, 3, 2, 3, 3, 3, 4 }
 },

    /*  The L   ++++++
                ++        */
 {
    {  2, 2, 2, 3, 3, 3, 4, 3 },
    {  3, 2, 3, 3, 3, 4, 4, 2 },
    {  2, 2, 3, 2, 4, 2, 4, 3 },
    {  2, 4, 3, 2, 3, 3, 3, 4 }
 },

    /*  The J   ++++++
                    ++    */
 {
    {  2, 3, 3, 3, 4, 2, 4, 3 },
    {  3, 2, 3, 3, 3, 4, 4, 4 },
    {  2, 2, 3, 2, 4, 2, 2, 3 },
    {  2, 2, 3, 2, 3, 3, 3, 4 }
 },

    /*  The S     ++++
                ++++      */
 {
    {  2, 2, 3, 2, 3, 3, 4, 3 },
    {  3, 3, 3, 4, 4, 2, 4, 3 },
    {  2, 2, 3, 2, 3, 3, 4, 3 },
    {  3, 3, 3, 4, 4, 2, 4, 3 }
 },

    /*  The Z   ++++
                  ++++    */
 {
    {  2, 3, 3, 2, 3, 3, 4, 2 },
    {  3, 2, 3, 3, 4, 3, 4, 4 },
    {  2, 3, 3, 2, 3, 3, 4, 2 },
    {  3, 2, 3, 3, 4, 3, 4, 4 }
 }

};

int bw = 0;
int funmode = 0;
int flashy = 0;
int standalone = 0;
int sqrsize = 24;
int hscroll = 10;
int paused = 0;
int visiblemsgs = 0, msgsclosed = 0;
int verbose = 0;
int norestart = 0;
int nospeedup = 0;

int depth;
HWND hwnd;
HDC hdc;
HPEN hpenOld;
HBRUSH hbrushOld;
HFONT hfontOld;
HPEN hpen;
HBRUSH hbrush;
HFONT hfont ;
PAINTSTRUCT ps;
int height, width, x_initted = 0;
COLORREF color[10][3], white, black;

// lccwin32 doesn't have this defined.
#ifndef VK_OEM_COMMA
#define VK_OEM_COMMA 188
#endif

int keysspecified = 0;
int quit_key = 'q', start_key = 's', pause_key = 'p', bot_key = 'b';
int text_key = 't', togglenext_key = VK_RETURN, mode_key = 'n';
int verbose_key = 'v', clear_key = 'c', warp_key = VK_TAB;
int down_key = VK_OEM_COMMA, zero_key = 'z';
int level_keys[10] =
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
int keys[6] = {
	'J', 'j', 'k', 'l', 'L', ' ' };

int warp_next = 1;

#define MAXNICKLEN 14

/* this needs to be a fixed-width font */
//#define FONT "-misc-fixed-bold-r-normal--13-*-*-*-*-*-*-*"
//#define FONT "Monospaced"
#define FONTWIDTH 7
#define FONTHEIGHT 13

/* position of all buttons & areas */

#define XBTNS 20
#define XLVLS 19
#define XSPEEDTXT 21
#define XNEXT 15
#define XNEXTTXT 16
#define XNICK (XMARGIN + (WPITIN-WNICK) / 2) /* relative to u->x0 */
#define XTXT 30
#define XZERO 30
#define XSCROLL (XPIT + LEFTSIDE)
#define XMSGENTRY (XPIT + LEFTSIDE)
#define XLINESTXT (XMARGIN + 25)
#define XLINES (XMARGIN + 80)
#define XGAMES XLINES
#define XGAMESTXT (XMARGIN + 25)

#define YQUIT (YMARGIN + 2)
#define YPLAY (YQUIT + 27)
#define YPAUSE (YPLAY + 27)
#define YBOT (YPAUSE + 27)
#define YSPEEDTXT (YBOT + 46)
#define YLVLS (YSPEEDTXT + 8)
#define YMODE (YLVLS + 87)
#define YNEXTTXT (YMODE + 44)
#define YNEXT (YNEXTTXT + 6)
#define YNICK (16 + YMARGIN + (HSQR * ROWS))
#define YTXT YNICK
#define YZERO (YTXT + 35)
#define YLINES (YNICK + HTEXTENTRIES + 13)
#define YLINESTXT (YLINES + 15)
#define YGAMES (YLINES + HTEXTENTRIES + 10)
#define YGAMESTXT (YGAMES + 15)
#define YSCROLL (HEIGHT0 - 17)
#define YMSGENTRY (YSCROLL + HSCROLL * FONTHEIGHT + 17)
#define YVERBOSE (YSCROLL + 20)
#define YCLEAR (YSCROLL + 55)


#define XMARGIN 8
#define YMARGIN 12

/* width & height of buttons & areas */

#define LEFTSIDE (46 + 4 * WSQR) /* size of the whole left-side buttons area */

#define WIDTH0 ((WSQR * COLS) + XMARGIN + 22)
#define HEIGHT0 (147 + YMARGIN + (HSQR * ROWS))

#define WSQR sqrsize
#define HSQR sqrsize

#define WBTNS 70
#define HBTNS 20
#define WTXT 50
#define WZERO 50
#define WNICKCHARS 16

#define WNICK (WNICKCHARS * FONTWIDTH + 8)
#define WL 20
#define HL 20
#define WNEXT (4 * WSQR + 16)
#define HNEXT (2 * HSQR + 12)
#define WSCROLL 50 /* for 2 users */
#define HSCROLL hscroll
#define WSCROLLRESIZE (X_EACH / FONTWIDTH)
#define HSCROLLRESIZE (HSCROLL * FONTHEIGHT + HTEXTENTRIES + 45)
#define WMSGENTRY (WSCROLL * FONTWIDTH + 8)
#define WLINES 60
#define HLINES HTEXTENTRIES
#define WGAMES WLINES
#define HGAMES HTEXTENTRIES
#define HTEXTENTRIES 23

/* don't change these: */

#define COLS 10
#define ROWS 20

#define X_EACH (WIDTH0 + 10)
#define XPIT (XMARGIN + 2)
#define YPIT (YMARGIN + 2)
#define WPITIN (COLS * WSQR)
#define HPITIN (ROWS * HSQR)
#define WPITOUT (COLS * WSQR + 4)
#define HPITOUT (ROWS * HSQR + 4)

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

typedef void (*v_v)(void);
typedef void (*v_vtp)(struct timeval *, void *);
typedef void (*v_si)(char *, int);

struct button {
	int x, y, w, h;
	int down, flash;
	char *txt;
	int align;
	v_v callback;
	struct button *next;
};

struct button *button0 = NULL;

struct toggle {
	int x, y, w, h;
	char *txt;
	v_v callback;
	int down;
	int align;
	struct toggle *next;
	struct toggle *master;
};

struct toggle *tog0 = NULL;

struct textentry {
	int x, y, w, h;
	int cols; /* in chars */
	char *txt;
	int maxlen;
	int offset;
	int align;
	int cursor;
	int next_clears;
	v_si callback;
	struct textentry *next;
};

struct textentry *text0 = NULL;

struct textscroll {
	int x, y, w, h;
	int rows, cols; /* in chars */
	char **lines, **plines;
	int nextpline, nextline;
	COLORREF color;
	struct textentry *redirect;
	struct textscroll *next;
};

struct textscroll *scroll0 = NULL;

struct timer {
	struct timeval tv;
	v_vtp action;
	void *data;
	struct timer *next;
};

struct timer *timer0 = NULL;

struct lostfalldata {
	struct user *u;
	int counter;
};

struct flashlinedata {
	struct user *u;
	int counter;
	char lines[20];
	int nlines;
	COLORREF color;
	int oldpit[20][10];
};

/* server stuff */

#define BUFLEN 4096
unsigned int sfd;
int port = 0;
unsigned char realbuf[512], readbuf[BUFLEN], writbuf[BUFLEN];
unsigned char *buf = realbuf + 4;
int nread = 0, nwrite = 0;
char *serverhost = NULL;

struct textentry *nickentry, *msgentry;
struct button *modebutton, *pausebutton, *quitbutton, *playbutton, *botbutton;
struct button *textbutton = NULL, *verbosebutton, *clearbutton, *zerobutton;
struct toggle *leveltogs[10];
struct textscroll *msgscroll;
int scrollcols; /* for 1 user */

struct user {
	unsigned char number;
	int x0;
	char nick[MAXNICKLEN+2];
	struct user *next;
	int pit[20][10];
	char haslost, flashing;
	char replaybuf[BUFLEN];
	int replaylen;
	int lines, games;
};

struct user me;

int delays[10] = {
	1500000, 1200000, 900000, 700000, 500000, 350000, 200000,
	120000, 80000 };

/* #define textwidth(text)  XTextWidth(font_info, text, strlen(text)) */
#define textwidth(text) (FONTWIDTH * strlen(text))

#define WINNAME "xtris v" VERSION

int oldpit[20][10], oldpittmp[20][10];
#define EMPTY	7
#define GRAY    8
#define TXT	9
int piece, rotation, x, y, next, shownext = 1, level = 5;
int playing = 0, got_play = 0, interrupt = 0, addlines = 0, addsquares = 0;


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
char winClass[] = "xtrisWindow";
BOOL ctrlDown, shiftDown;
#define USERNAME_SIZE 64
int xLastPressed;
int yLastPressed;
void runGame(void);
void processKeyEvent(int vkCode);
void handlemessages(void);
void c_quit(void);
int status = 0;
LOGFONT logfont;
void transformToWinColors(void);
BOOL needsRepaint = FALSE;

/* like memcpy, but guaranteed to handle overlap when s <= t */
void copydown(char *s, char *t, int n) {
	for (; n; n--)
		*(s++) = *(t++);
}

void fatal(char *s) {
	fprintf(stderr, "%s.\n", s);
	status = -1;
	c_quit();
}

void *mmalloc(int n) {
	void *r;

	r = malloc(n);
	if (r == NULL)
		fatal("Out of memory");
	return r;
}

void u_sleep(int i) {
	Sleep(i / 1000);
}

void flushbuf(void) {
	if (nwrite) {
		send(sfd, writbuf, nwrite, 0);
		nwrite = 0;
	}
}

void sendbuf(int len) {
	if (!standalone) {
		realbuf[0] = realbuf[1] = realbuf[2] = 0;
		realbuf[3] = (unsigned char)len;
		buf[0] = 0;
		if (nwrite + 4 + len >= BUFLEN)
			fatal("Internal error: send buffer overflow");
		memcpy(writbuf + nwrite, realbuf, 4 + len);
		nwrite += 4 + len;
	}
}


void getcolors(int light, int normal, int dark, COLORREF *color) {
	color[0] = light;
	color[1] = normal;
	color[2] = dark;
}

HWND initWindow(void) {
    HINSTANCE hinst = GetModuleHandle(NULL);
	HWND hwndMain;
	WNDCLASS wc;

	width = WIDTH0 + LEFTSIDE;
	height = HEIGHT0;

	memset(&wc,0,sizeof(wc));
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hinst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
//	wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wc.hbrBackground = CreateSolidBrush(color[EMPTY][1]);
	wc.lpszClassName = winClass;
	RegisterClass(&wc);

//	win_name = mmalloc(strlen(WINNAME) + 2);
//	strcpy(win_name, WINNAME);
    //hwnd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, DialogFunc);
    hwndMain = CreateWindowEx( 0, winClass, WINNAME,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hinst, NULL);

    if(hwndMain == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwndMain, SW_SHOW);
    UpdateWindow(hwndMain);

    return hwndMain;
}

void init_X(void) {
	int n;

	depth = 16;
	width = WIDTH0 + LEFTSIDE;
	height = HEIGHT0;
	white = 0xffffff;
	black = 0x000000;

	getcolors(0xe1e1e1, 0xb2b2b2, 0x656565, color[EMPTY]);

	color[TXT][0] = white;
	color[TXT][1] = color[EMPTY][1];
	color[TXT][2] = 0x404040;

	color[GRAY][0] = color[EMPTY][0];
	color[GRAY][1] = color[EMPTY][1];
	color[GRAY][2] = color[EMPTY][2];

	hwnd = initWindow();

	hdc = GetDC(hwnd);

	memset(&logfont, 0, sizeof(logfont));
	logfont.lfHeight = (LONG)(-FONTHEIGHT * 0.99);
	logfont.lfWidth = FONTWIDTH;
	logfont.lfWeight = FW_BOLD;
	logfont.lfPitchAndFamily = FF_MODERN | FIXED_PITCH;
	sprintf(logfont.lfFaceName, "Monospaced");

	hfontOld = GetCurrentObject(hdc, OBJ_FONT);
	hfont = CreateFontIndirect(&logfont);
	SelectObject(hdc, hfont);


	if (depth > 4 && !bw) {
		if (flashy) {

			color[0][0] = 0xfa0000;
			color[0][1] = 0xbc0000;     /* dark red */
			color[0][2] = 0x7d0000;

			color[1][0] = 0x00fa00;
			color[1][1] = 0x00bc00;     /* green */
			color[1][2] = 0x007d00;

			color[2][0] = 0x0000fa;
			color[2][1] = 0x0000bc;     /* dark blue */
			color[2][2] = 0x00007c;

			color[3][0] = 0xfafa00;
			color[3][1] = 0xbcbc00;     /* yellow */
			color[3][2] = 0x7d7d00;

			color[4][0] = 0x00fafa;
			color[4][1] = 0x00bcbc;     /* light blue-green */
			color[4][2] = 0x007d7d;

			color[5][0] = 0xfa00fa;
			color[5][1] = 0xbc00bc;     /* mauve */
			color[5][2] = 0x7d007d;

			color[6][0] = 0xbc00fa;
			color[6][1] = 0x7d00bc;     /* dark violetish blue */
			color[6][2] = 0x1f007d;

		}
		else {

			/* dark red */
			getcolors(0xfa0000, 0xbc0000, 0x7d0000, color[0]);

			/* green */
			getcolors(0x00f000, 0x00a800, 0x007000, color[1]);

			/* blue */
			getcolors(0x4080f0, 0x0060d8, 0x00407c, color[2]);

			/* brownish */
			getcolors(0xd0b000, 0x968000, 0x706000, color[3]);

			/* dark blue-green */
			getcolors(0x00b0b0, 0x008080, 0x006060, color[4]);

			/* mauve */
			getcolors(0xfa00fa, 0xbc00bc, 0x7d007d, color[5]);

			/* dark violetish blue */
			getcolors(0xbc00fa, 0x7d00bc, 0x1f007d, color[6]);

		}
	}
	else if (depth > 4 && bw) {
		color[0][0] = 0xe0e0e0;
		color[0][1] = 0x606060;
		color[0][2] = black;
		for (n=1; n<7; n++) {
			color[n][0] = color[0][0];
			color[n][1] = color[0][1];
			color[n][2] = black;
		}
	}
	else {
		for (n=0; n<7; n++) {
     color[n][0] = white;
			color[n][1] = black;
			color[n][2] = black;
		}
	}
  transformToWinColors();
}

// NEW, windows colors are defined: 0x00bbggrr
//      x windows                   0x00rrggbb
void transformToWinColors(void) {
  int n, i;
  COLORREF tmpColor;
  COLORREF maskRed = 0x000000ff;
  COLORREF maskGreen = 0x0000ff00;
  COLORREF maskBlue = 0x00ff0000;
  
  for(n=0; n<10; n++) {
    for(i=0; i<3; i++) {
      tmpColor = color[n][i];
      color[n][i] = tmpColor & maskGreen; // stays on same place
      color[n][i] |= ((tmpColor & maskRed) << 16);
      color[n][i] |= ((tmpColor & maskBlue) >> 16);
    }
  }
}

void RestoreColors(void) {
	if(hdc != NULL) {
		if(hpen != NULL && hpenOld != NULL) {
			SelectObject(hdc, hpenOld);
			DeleteObject(hpen);
			hpen = NULL;
		}
		if(hbrush != NULL && hbrushOld != NULL) {
			SelectObject(hdc, hbrushOld);
			DeleteObject(hbrush);
			hbrush = NULL;
		}
	}
}

void SetColor(COLORREF col) {
	if(hdc != NULL) {
		// In case we didn't call it since last call to SetColor.
		RestoreColors();

		hpenOld = GetCurrentObject(hdc, OBJ_PEN);
		hpen = CreatePen(PS_SOLID, 1, col);
		SelectObject(hdc, hpen);

		hbrushOld = GetCurrentObject(hdc, OBJ_BRUSH);
		hbrush = CreateSolidBrush(col);
		SelectObject(hdc, hbrush);

		SetBkColor(hdc, color[EMPTY][1]);
		SetTextColor(hdc, col);
	}
}

void XFillRectangle(int x, int y, int w, int h) {
	if(hdc != NULL) {
		RECT rect;
		rect.left = x;
		rect.top = y;
		rect.right = x + w;
		rect.bottom = y + h;
		FillRect(hdc, &rect, hbrush);
	}
}

void XDrawLine(int x1, int y1, int x2, int y2) {
	if(hdc != NULL) {
		MoveToEx(hdc, x1, y1, NULL);
		LineTo(hdc, x2, y2);
	}
}

//void XDrawString(display, win, gc, xx, y+FONTHEIGHT+2 , text, strlen(text)) {
void XDrawString(int x, int y, char *text, int len) {
	if(hdc != NULL) {
		TextOut(hdc, x, y - FONTHEIGHT, text, len);
	}
}

void XWarpPointer(int dest_x, int dest_y) {
	POINT p;
	p.x = dest_x;
	p.y = dest_y;
	ClientToScreen(hwnd, &p);
	SetCursorPos(p.x, p.y);
}

void XFlush(void) {
  GdiFlush();
}

void DrawSqr(struct user *u, int type, int x, int y) {
	if (!u) {
		/* fixed position for "next"-type squares */
		x = (x * WSQR) + XNEXT + 6;
		y = (y * HSQR) + YNEXT + 5;
	}
	else {
		x = (x * WSQR) + XPIT + u->x0;
		y = (y * HSQR) + YPIT;
	}
	type &= 0xff;
	if (type == EMPTY) {
		SetColor(color[EMPTY][1]);
		XFillRectangle(x, y, WSQR, HSQR);
		RestoreColors();
	}
	else {
		SetColor(color[type][1]);
		XFillRectangle(x+2, y+2, WSQR-4, HSQR-4);
		SetColor(color[type][0]);
		XDrawLine(x, y, x, y+HSQR-1);
		XDrawLine(x, y, x+WSQR-1, y);
		XDrawLine(x+1, y, x+1, y+HSQR-1);
		XDrawLine(x, y+1, x+WSQR-1, y+1);
		SetColor(color[type][2]);
		XDrawLine(x+WSQR-2, y+2, x+WSQR-2, y+HSQR-2);
		XDrawLine(x+WSQR-1, y+1, x+WSQR-1, y+HSQR-1);
		XDrawLine(x, y+HSQR-1, x+WSQR-1, y+HSQR-1);
		XDrawLine(x+1, y+HSQR-2, x+WSQR-2, y+HSQR-2);
		RestoreColors();
	}
}

void DrawLine(struct user *u, int type, int y) {
	int x;

	x = u->x0 + XPIT;
	y = (y * HSQR) + YPIT;

	SetColor(color[type][1]);
	XFillRectangle(x+2, y+2, WPITIN-4, HSQR-4);
	SetColor(color[type][0]);
	XDrawLine(x, y, x, y+HSQR-1);
	XDrawLine(x, y, x+WPITIN-1, y);
	XDrawLine(x+1, y, x+1, y+HSQR-1);
	XDrawLine(x, y+1, x+WPITIN-1, y+1);
	SetColor(color[type][2]);
	XDrawLine(x+WPITIN-2, y+2, x+WPITIN-2, y+HSQR-2);
	XDrawLine(x+WPITIN-1, y+1, x+WPITIN-1, y+HSQR-1);
	XDrawLine(x, y+HSQR-1, x+WPITIN-1, y+HSQR-1);
	XDrawLine(x+1, y+HSQR-2, x+WPITIN-2, y+HSQR-2);
	RestoreColors();
}

void DrawButton(int updown, int bar, int x, int y, int w, int h, char *text, int align, int reverse) {
	int u, l, xx;

	if (x > width || y > height)
		return;
	if (updown) {
		u = 2;
		l = 0;
	}
	else {
		u = 0;
		l = 2;
	}
	if (bar) {
		SetColor(color[EMPTY][1]);
		XFillRectangle(x+2, y+2, w-4, h-4);
	}

	SetColor(color[EMPTY][u]);
	XDrawLine(x, y, x, y+h-1);
	XDrawLine(x, y, x+w-1, y);
	XDrawLine(x+1, y, x+1, y+h-1);
	XDrawLine(x, y+1, x+w-1, y+1);
	SetColor(color[EMPTY][l]);
	XDrawLine(x+w-2, y+2, x+w-2, y+h-2);
	XDrawLine(x+w-1, y+1, x+w-1, y+h-1);
	XDrawLine(x, y+h-1, x+w-1, y+h-1);
	XDrawLine(x+1, y+h-2, x+w-2, y+h-2);

	if (text) {
		if (reverse)
			SetColor(color[TXT][u]);
		else
		    SetColor(color[TXT][l]);
		switch (align) {
		case ALIGN_LEFT:
			xx = x + 4;
			break;
		case ALIGN_CENTER:
			xx = x + ((w - textwidth(text)) >> 1);
			break;
		default:
			xx = x + w - textwidth(text) - 6;
			break;
		}
		XDrawString(xx, y+FONTHEIGHT+2, text, strlen(text));
	}
	RestoreColors();
}

void WriteXY(int x, int y, char *txt) {
	XDrawString(x, y, txt, strlen(txt));
}

void UpdateWinSize(void) {
	RECT rect;
	GetWindowRect(hwnd, &rect);
	MoveWindow(hwnd, rect.left, rect.top, width, height, TRUE);
}

struct button *DeclareButton(int x, int y, int w, int h, int down, int flash,
	char *txt, int align, v_v callback) {
	struct button *b;

	b = mmalloc(sizeof (struct button));
	b->x = x;
	b->y = y;
	b->w = w;
	b->h = h;
	b->down = down;
	b->flash = flash;
	b->txt = txt;
	b->align = align;
	b->callback = callback;
	b->next = button0;
	button0 = b;

	return b;
}

void RemoveButton(struct button *b) {
	struct button *bb;

	if (b == button0) {
		button0 = button0->next;
	}
	else {
		for (bb=button0; bb->next; bb=bb->next) {
			if (bb->next == b) {
				bb->next = bb->next->next;
				break;
			}
		}
	}

	SetColor(color[EMPTY][1]);
	XFillRectangle(b->x, b->y, b->w, b->h);
	RestoreColors();
	XFlush();
}

struct toggle *DeclareToggle(int x, int y, int w, int h, char *txt, int align,
	v_v callback, struct toggle *master) {
	struct toggle *t;

	t = mmalloc(sizeof (struct toggle));
	t->x = x;
	t->y = y;
	t->w = w;
	t->h = h;
	t->txt = txt;
	t->align = align;
	t->callback = callback;
	t->next = tog0;
	t->down = (master == NULL);
	t->master = master == NULL ? t : master;
	tog0 = t;

	return t;
}

struct textentry *DeclareTextEntry(int x, int y, int cols, int maxlen,
	char *txt, int cursor, v_si callback) {
	struct textentry *t;
	char *s;

	s = mmalloc(5 + maxlen);
	t = mmalloc(sizeof(struct textentry));
	t->x = x;
	t->y = y;
	t->w = cols * FONTWIDTH + 8;
	t->cols = cols;
	t->h = HTEXTENTRIES;
	t->txt = s;
	t->next_clears = 0;
	t->offset = 0;
	t->cursor = cursor;
	strncpy(s, txt, maxlen);
	t->maxlen = maxlen;
	t->callback = callback;
	t->next = text0;
	text0 = t;

	return t;
}

struct textscroll *DeclareTextScroll(int x, int y, int cols, int rows,
	int color, struct textentry *redirect) {
	struct textscroll *t;
	int i;

	t = mmalloc(sizeof(struct textscroll));
	t->x = x;
	t->y = y;
	t->w = cols * FONTWIDTH + 8;
	t->h = rows * FONTHEIGHT + 8;
	t->rows = rows;
	t->cols = cols;
	t->lines = mmalloc(rows * sizeof(char *));
	t->plines = mmalloc(rows * sizeof(char *));
	for (i=0; i<rows; i++)
		t->lines[i] = t->plines[i] = NULL;
	t->nextpline = t->nextline = 0;
	t->color = color;
	t->next = scroll0;
	t->redirect = redirect;
	scroll0 = t;

	return t;
}

void DrawScrollTextLine(struct textscroll *t, int line) {
	SetColor(t->color);
	if (t->plines[line])
		WriteXY(t->x + 4, t->y + 15 + FONTHEIGHT * line, t->plines[line]);
	RestoreColors();
}

void DrawScrollText(struct textscroll *t) {
	int i;

	DrawButton(1, 1, t->x, t->y, t->w, t->h, NULL, ALIGN_LEFT, 0);
	for (i=0; i<t->rows; i++)
		DrawScrollTextLine(t, i);
}

void AddScrollPline(struct textscroll *t, char *s, int update) {
	char *ss;
	int i;

	ss = mmalloc(1 + strlen(s));
	strcpy(ss, s);

	if (t->nextpline >= t->rows) {
		if (t->plines[0])
			free(t->plines[0]);
		for (i=0; i<t->rows-1; i++)
			t->plines[i] = t->plines[i+1];
		t->plines[t->rows - 1] = ss;
		if (update)
			DrawScrollText(t);
	}
	else {
		t->plines[t->nextpline] = ss;
		if (update)
			DrawScrollTextLine(t, t->nextpline);
		t->nextpline++;
	}
	if (update)
		XFlush();
}

void AddLineToPlines(struct textscroll *t, char *s, int update) {
	char *p, *w, *lww, *lws;

	lww = w = p = mmalloc(2 + t->cols);

	while (*s) {
		if (w - p >= t->cols - 1) {
			if (*s == ' ') {
				lww = w;
			}
			else if (lww > p + 7) {
				w = lww;
				s = lws;
			}
			*w = 0;
			AddScrollPline(t, p, update);
			w = p;
			*w++ = ' ';
			*w++ = ' ';
			*w++ = ' ';
			*w++ = ' ';
			*w++ = ' ';
			*w++ = ' ';
			lww = w;
		}
		*w++ = *s;
		if (*s++ == ' ') {
			lws = s;
			lww = w;
		}
	}
	if (w > p) {
		*w = 0;
		AddScrollPline(t, p, update);
	}
	free(p);
}

void RecalcPlines(struct textscroll *t) {
	int i;

	for (i=0; i<t->rows; i++) {
		if (t->plines) {
			free(t->plines[i]);
			t->plines[i] = NULL;
		}
	}
	t->nextpline = 0;
	for (i=0; i<t->rows; i++)
		if (t->lines[i] != NULL)
			AddLineToPlines(t, t->lines[i], 0);
}

void ClearScrollArea(struct textscroll *t) {
	int i;

	for (i=0; i<t->rows; i++) {
		if (t->plines) {
			free(t->plines[i]);
			t->plines[i] = NULL;
		}
		if (t->lines) {
			free(t->lines[i]);
			t->lines[i] = NULL;
		}
	}
	t->nextpline = 0;
	t->nextline = 0;
	DrawButton(1, 1, t->x, t->y, t->w, t->h, NULL, ALIGN_LEFT, 0);
}

void SetTextScrollWidth(struct textscroll *t, int cols) {
	t->cols = cols;
	t->w = cols * FONTWIDTH + 8;
	RecalcPlines(t);
}

void SetTextEntryWidth(struct textentry *t, int cols) {
	int len;

	len = strlen(t->txt);
	t->cols = cols;
	t->w = cols * FONTWIDTH + 8;
	if (len - t->offset >= t->cols)
		t->offset = len - t->cols + 3;
}

void AddScrollLine(struct textscroll *t, char *s) {
	char *ss;
	int i;

	ss = mmalloc(1 + strlen(s));
	strcpy(ss, s);

	if (t->nextline >= t->rows) {
		if (t->lines[0])
			free(t->lines[0]);
		for (i=0; i<t->rows-1; i++)
			t->lines[i] = t->lines[i+1];
		t->lines[t->rows - 1] = ss;
	}
	else {
		t->lines[t->nextline] = ss;
		t->nextline++;
	}
	AddLineToPlines(t, s, 1);
}

void RedrawButtons(void) {
	struct button *b;

	for (b=button0; b; b=b->next)
		DrawButton(b->down, 1, b->x, b->y, b->w, b->h, b->txt, b->align, 0);
}

void RedrawToggles(void) {
	struct toggle *t;

	for (t=tog0; t; t=t->next)
		DrawButton(t->down, 1, t->x, t->y, t->w, t->h, t->txt, t->align, 0);
}

void RedrawTextEntryArea(struct textentry *t) {
	int l;

	DrawButton(1, 1, t->x, t->y, t->w, t->h, t->txt + t->offset, t->align, t->next_clears ? 0 : 1);
	if (t->cursor) {
		l = strlen(t->txt) - t->offset;
		SetColor(color[TXT][2]);
		XDrawLine(t->x + FONTWIDTH * l + 6, t->y + 4, t->x + FONTWIDTH * l + 6, t->y + 16);
		RestoreColors();
	}
	XFlush();
}

void RedrawTextEntries(void) {
	struct textentry *t;

	for (t=text0; t; t=t->next)
		RedrawTextEntryArea(t);
}

void RedrawTextScrolls(void) {
	struct textscroll *t;

	for (t=scroll0; t; t=t->next)
		DrawScrollText(t);
}

void ToggleDown(struct toggle *t) {
	struct toggle *tt;

	for (tt=tog0; tt; tt=tt->next) {
		if (tt != t && tt->down && tt->master == t->master) {
			tt->down = 0;
			DrawButton(0, 0, tt->x, tt->y, tt->w, tt->h, tt->txt, tt->align, 0);
			XFlush();
		}
	}
	if (!t->down) {
		t->down = 1;
		DrawButton(1, 0, t->x, t->y, t->w, t->h, t->txt, t->align, 0);
		XFlush();
	}
}

void ChangeButtonAppearance(struct button *b, int down, char *txt, int align) {
	/* down == -1 => no change;  txt == NULL => no change; align == -1 => no change */

	if (down >= 0)
		b->down = down;
	if (txt != NULL)
		b->txt = txt;
	if (align >= 0)
		b->align = align;
	DrawButton(b->down, 1, b->x, b->y, b->w, b->h, b->txt, b->align, 0);
	XFlush();
}

void ChangeTextEntryArea(struct textentry *t, char *txt, int align, int cursor) {
	/* txt == NULL => no change;  align == -1 => no change;  cursor == -1 => no change */
	if (align >= 0)
		t->align = align;
	if (txt != NULL)
		strncpy(t->txt, txt, t->maxlen);
	if (cursor >= 0)
		t->cursor = cursor;
	RedrawTextEntryArea(t);
}

void FlashButton(struct button *b) {
	DrawButton(!b->down, 0, b->x, b->y, b->w, b->h, b->txt, b->align, 0);
	XFlush();
	u_sleep(100000);
	DrawButton(b->down, 0, b->x, b->y, b->w, b->h, b->txt, b->align, 0);
	XFlush();
}

void SimulateButtonPress(struct button *b) {
	if (b->flash)
		FlashButton(b);
	b->callback();
}

void SimulateTextEntry(struct textentry *t, char *txt) {
	strncpy(t->txt, txt, t->maxlen);
	t->callback(t->txt, 0);
	t->next_clears = 1;
}

void updatelines(struct user *u) {
	static char tmpbuf[12];

	sprintf(tmpbuf, "%d", u->lines);
	DrawButton(1, 1, XLINES + u->x0, YLINES, WLINES, HLINES, tmpbuf, ALIGN_RIGHT, 1);
}

void updategames(struct user *u) {
	static char tmpbuf[12];

	sprintf(tmpbuf, "%d", u->games);
	DrawButton(1, 1, XGAMES + u->x0, YGAMES, WGAMES, HGAMES, tmpbuf, ALIGN_RIGHT, 1);
}

void DrawFrame(void) {
	struct user *u;

	/* the Next square */
	DrawButton(1, 0, XNEXT, YNEXT, WNEXT, HNEXT, NULL, ALIGN_LEFT, 0);
	SetColor(color[TXT][0]);
	WriteXY(XNEXTTXT, YNEXTTXT, "Next:");

	WriteXY(XSPEEDTXT, YSPEEDTXT, "Speed:");

	RedrawButtons();
	RedrawToggles();
	RedrawTextEntries();
	RedrawTextScrolls();

	for (u=&me; u; u=u->next) {
		DrawButton(1, 0, XMARGIN + u->x0, YMARGIN, WPITOUT, HPITOUT, NULL, ALIGN_LEFT, 0);
		if (u != &me)  /* me.nick is actually a textarea */
			DrawButton(1, 0, XNICK + u->x0, YNICK, WNICK, HTEXTENTRIES, u->nick, ALIGN_CENTER, 0);
		SetColor(color[TXT][0]);
		WriteXY(XLINESTXT + u->x0, YLINESTXT, "Lines:");
		WriteXY(XGAMESTXT + u->x0, YGAMESTXT, "Games:");
		updatelines(u);
		updategames(u);
	}
	RestoreColors();
}

/* a random number generator loosely based on RC5;
assumes ints are at least 32 bit */

unsigned int my_rand(void) {
	static unsigned int s = 0, t = 0, k = 12345678;
	int i;

	if (s == 0 && t == 0) {
		s = (unsigned int)getpid();
		t = (unsigned int)time(NULL);
	}
	for (i=0; i<12; i++) {
		s = (((s^t) << (t&31)) | ((s^t) >> (31 - (t&31)))) + k;
		k += s + t;
		t = (((t^s) << (s&31)) | ((t^s) >> (31 - (s&31)))) + k;
		k += s + t;
	}
	return s;
}

int fits(int piece, int rotation, int x, int y) {
	int xx, yy, i;

	for (i=0; i<4; i++) {
		xx = x + shape[piece][rotation][2*i];
		yy = y - shape[piece][rotation][2*i+1];
		if (xx < 0 || xx > 9 || yy > 19)
			return 0;
		if (yy >= 0 && me.pit[yy][xx] != EMPTY)
			return 0;
	}
	return 1;
}

int sticksout(int piece, int rotation, int x, int y) {
	int i;

	for (i=0; i<4; i++)
		if (y - shape[piece][rotation][2*i+1] < 0)
			return 1;
	return 0;
}

void put(int piece, int rotation, int x, int y, int color) {
	int xx, yy, i;

	for (i=0; i<4; i++) {
		xx = x + shape[piece][rotation][2*i];
		yy = y - shape[piece][rotation][2*i+1];
		if (yy >= 0)
			me.pit[yy][xx] = color;
	}
}

void drawnext(void) {
	int i, xx, yy;

	if (shownext)
		for (i=0; i<4; i++) {
			xx = shape[next][0][2*i];
			yy = 3 - shape[next][0][2*i+1];
			DrawSqr(NULL, next, xx-1, yy);
		}
}

void clearnext(void) {
	SetColor(color[EMPTY][1]);
	XFillRectangle(XNEXT+3, YNEXT+3, WNEXT-6, HNEXT-6);
	RestoreColors();
}

#define remove(p, r, x, y) put(p, r, x, y, EMPTY)

void refresh(void) {
	int x, y;
	int i = 2;

	buf[1] = OP_DRAW;

	for (x=0; x<10; x++)
		for (y=0; y<20; y++)
			if (me.pit[y][x] != oldpit[y][x]) {
				DrawSqr(&me, me.pit[y][x], x, y);
				if (me.pit[y][x] != (oldpit[y][x] & 0xff)) {
					buf[i] = x;
					buf[i+1] = y;
					buf[i+2] = me.pit[y][x];
					i += 3;
					if (i > 200) {
						sendbuf(i);
						i = 2;
						buf[1] = OP_DRAW;
					}
				}
				oldpit[y][x] = me.pit[y][x];
			}
	if (i > 2)
		sendbuf(i);
}

void refreshnet(void) {
	int x, y;
	int i = 2;

	buf[1] = OP_DRAW;

	for (x=0; x<10; x++)
		for (y=0; y<20; y++)
			if (me.pit[y][x] != oldpit[y][x]) {
				if (me.pit[y][x] != (oldpit[y][x] & 0xff)) {
					buf[i] = x;
					buf[i+1] = y;
					buf[i+2] = me.pit[y][x];
					i += 3;
					if (i > 200) {
						sendbuf(i);
						i = 2;
						buf[1] = OP_DRAW;
					}
				}
				oldpit[y][x] = me.pit[y][x];
			}
	if (i > 2)
		sendbuf(i);
}

void refreshme(void) {
	int x, y;

	for (x=0; x<10; x++)
		for (y=0; y<20; y++)
			if (me.pit[y][x] != oldpit[y][x]) {
				DrawSqr(&me, me.pit[y][x], x, y);
				oldpit[y][x] = me.pit[y][x];
			}
}

void refreshuser(struct user *u) {
	int x, y, n = 0;

	for (x=0; x<10; x++)
		for (y=0; y<20; y++)
			if (u->pit[y][x] != oldpittmp[y][x]) {
				DrawSqr(u, u->pit[y][x], x, y);
				n++;
			}
	if (n)
		XFlush();
}

void redrawall() {
	int x, y;
	struct user *u;

	DrawFrame();
	for (u=&me; u; u=u->next)
		for (x=0; x<10; x++)
			for (y=0; y<20; y++)
				DrawSqr(u, u->pit[y][x], x, y);
	if (playing)
		drawnext();
  XFlush();
}

void copyline(struct user *u, int i, int j) {
	int n;

	for (n=0; n<10; n++)
		u->pit[j][n] = u->pit[i][n];
}

void emptyline(struct user *u, int i) {
	int n;

	for (n=0; n<10; n++)
		u->pit[i][n] = EMPTY;
}

void falline(struct user *u, int i) {
	int x;
	for (x = i-1; x>=0; x--)
		copyline(u, x, x+1);
	emptyline(u, 0);
}

void growline(struct user *u) {
	int n;
	for (n=0; n<19; n++)
		copyline(u, n+1, n);
	emptyline(u, 19);
}

void fallines(void) {
	int n, c, x, i, fallen = 0, nextcol;

	buf[1] = OP_FALL;
	nextcol = my_rand() % 7;
	for (n=0; n<20; n++) {
		c = 0;
		for (x=0; x<10; x++)
			if (me.pit[n][x] != EMPTY)
				c++;
		if (c == 10) {
			DrawLine(&me, nextcol, n);
			falline(&me, n);
			for (i=0; i<10; i++)
				oldpit[n][i] |= 0x100;
			buf[2 + fallen++] = (unsigned char)n;
			me.lines++;
		}
	}
	if (fallen) {
		sendbuf(2 + fallen);
		flushbuf();
		XFlush();
		u_sleep(80000);
		nextcol = (nextcol+1) % 7;
		for (i=2; i<fallen+2; i++)
			DrawLine(&me, nextcol, buf[i]);
		XFlush();
		u_sleep(80000);
		nextcol = (nextcol+1) % 7;
		for (i=2; i<fallen+2; i++)
			DrawLine(&me, nextcol, buf[i]);
		u_sleep(80000);
		XFlush();
		refreshme();
		updatelines(&me);
		XFlush();
	}
	if (fallen > 1 || (funmode && fallen > 0)) {
		buf[1] = OP_LINES;
		buf[2] = (unsigned char)fallen;
		sendbuf(3);
	}
}

int newlines(void) {
	int i = addlines, j, k, r = 1;

	if (playing) {
		while (i > 0) {
			for (j=0; j<10; j++)
				if (me.pit[0][j] != EMPTY)
					r = 0;
			growline(&me);
			buf[1] = OP_GROW;
			sendbuf(2);
			k = 0;
			for (j=0; j<10; j++)
				if (my_rand() % 7 < 3) {
					me.pit[19][j] = my_rand() % 7;
					k++;
				}
			if (k == 10)
				me.pit[19][my_rand() % 10] = EMPTY;
			i--;
		}
		if (addlines > 0) {
			refreshme();
			for (j=20-addlines; j<20; j++)
				for (k=0; k<10; k++)
					oldpit[j][k] = EMPTY;
			refreshnet();
			XFlush();
		}
		addlines = 0;
	}
	return r;
}

int newsquares(void) {
	int n = addsquares, i, j, k, r = 1;

	if (playing) {
		while (n > 0) {
			for (i=0; i<10; i++)
				if (me.pit[0][i] != EMPTY)
					r = 0;
			for (i=0; i<20; i++) {
				k = 0;
				for (j=0; j<10; j++) {
					if (me.pit[i][j] != EMPTY)
						k++;
				}
				if (k)
					break;
			}
			if (k == 0 || i == 0) {
				i = 19;
				j = my_rand() % 10;
			}
			else {
				j = my_rand() % k;
				for (k=0; k<10; k++) {
					if (me.pit[i][k] != EMPTY)
						if (j == 0) {
							me.pit[i-1][k] = GRAY;
							break;
						}
					else j--;
				}
			}
			n--;
		}
		if (addsquares > 0) {
			refresh();
			XFlush();
		}
		addsquares = 0;
	}
	return r;
}

#define sysmsg(s) msgfrom(NULL, s)

void msgfrom(struct user *u, char *s) {
	static char msgbuf[300];

	if (u)
		sprintf(msgbuf, "(%d) (%s)  %s", u->number, u->nick, s);
	else
	    sprintf(msgbuf, "** %s", s);
	if (u || verbose) {
		AddScrollLine(msgscroll, msgbuf);
		if (!visiblemsgs && !msgsclosed && me.next != NULL) {
			height += HSCROLLRESIZE;
			visiblemsgs = 1;
			UpdateWinSize();
		}
	}
}

void c_clear(void) {
	ClearScrollArea(msgscroll);
}

void c_msgentry(char *s, int shift) {
	buf[1] = OP_MSG;
	strcpy(&buf[2], s);
	sendbuf(strlen(s) + 2);
	msgfrom(&me, s);
	ChangeTextEntryArea(msgentry, "", ALIGN_LEFT, 1);
	if (shift) {
		XWarpPointer(3, 3);
		warp_next = 1;
	}
}

void c_txt(void) {
	if (visiblemsgs) {
		height -= HSCROLLRESIZE;
		visiblemsgs = 0;
		msgsclosed = 1;
	}
	else {
		height += HSCROLLRESIZE;
		visiblemsgs = 1;
	}
	UpdateWinSize();
	redrawall();
}

void c_zero(void) {
	if (standalone) {
		me.games = 0;
		updategames(&me);
		XFlush();
	}
	else {
		buf[1] = OP_ZERO;
		sendbuf(2);
	}
}

void c_quit(void) {
	if(hfont != NULL && hfontOld != NULL) {
		SelectObject(hdc, hfontOld);
		DeleteObject(hfont);
		hfont = NULL;
	}
	WSACleanup();
	exit(status);
}

void c_play(void) {
	if (standalone) {
		got_play = interrupt = 1;
	}
	else {
		buf[1] = OP_PLAY;
		sendbuf(2);
	}
}

void c_pause(void) {
	if (standalone) {
		paused = !paused;
		if (paused)
			ChangeButtonAppearance(pausebutton, 1, NULL, -1);
		else
		    ChangeButtonAppearance(pausebutton, 0, NULL, -1);
	}
	else {
		buf[1] = paused ? OP_CONT : OP_PAUSE;
		sendbuf(2);
	}
}

void c_verbose(void) {
	verbose = !verbose;
	if (verbose)
		ChangeButtonAppearance(verbosebutton, 1, NULL, -1);
	else
	    ChangeButtonAppearance(verbosebutton, 0, NULL, -1);
}

void c_bot(void) {
	int tmp;
	//VV 20040524 Port must be passed to bot...
	char portStr[16];
	sprintf(portStr, "%d", port);
	tmp = spawnlp(P_DETACH,
	#ifdef XTRISPATH
		XTRISPATH "/xtbot.exe",
	#else
		"xtbot.exe",
	#endif
		"xtbot", "-quiet", serverhost, portStr, NULL);

	if(tmp == -1) {
		fprintf(stderr, "Can't start bot '%s'.\n",
	#ifdef XTRISPATH
		XTRISPATH "/xtbot.exe");
	#else
		"xtbot.exe");
	#endif
	}
}

void c_setnick(char *s, int shift) {
	static char msgbuf[300];

	strncpy(me.nick, s, MAXNICKLEN);
	buf[1] = OP_NICK;
	memcpy(&buf[2], me.nick, strlen(me.nick));
	sendbuf(2 + strlen(me.nick));
	ChangeTextEntryArea(nickentry, s, ALIGN_CENTER, 0);
	sprintf(msgbuf, "you set your nick to %s", s);
	sysmsg(msgbuf);
	if (shift) {
		XWarpPointer(3, 3);
		warp_next = 1;
	}
}

void setlevel(int n) {
	if (n < 1)
		n = 1;
	if (n > 9)
		n = 9;
	level = n;
	ToggleDown(leveltogs[n]);
}

void set_mode(int n) {
	funmode = (n != 0);
	if (funmode)
		ChangeButtonAppearance(modebutton, -1, "fuN", -1);
	else
	    ChangeButtonAppearance(modebutton, -1, "Normal", -1);
}

void sendlevel(int n) {
	if (standalone)
		setlevel(n);
	else {
		buf[1] = OP_LEVEL;
		buf[2] = n;
		sendbuf(3);
	}
}

void sendmode(int n) {
	if (standalone)
		set_mode(n);
	else {
		buf[1] = OP_MODE;
		buf[2] = n;
		sendbuf(3);
	}
}

void c_speed1(void) {
	sendlevel(1);
}

void c_speed2(void) {
	sendlevel(2);
}

void c_speed3(void) {
	sendlevel(3);
}

void c_speed4(void) {
	sendlevel(4);
}

void c_speed5(void) {
	sendlevel(5);
}

void c_speed6(void) {
	sendlevel(6);
}

void c_speed7(void) {
	sendlevel(7);
}

void c_speed8(void) {
	sendlevel(8);
}

void c_speed9(void) {
	sendlevel(9);
}

void c_mode(void) {
	sendmode(funmode ? 0 : 1);
	if (!standalone) {
		if (funmode)
			ChangeButtonAppearance(modebutton, -1, "fuN", -1);
		else
		    ChangeButtonAppearance(modebutton, -1, "Normal", -1);
	}
}

void textentrykey(struct textentry *t, int keysym) {
	int len, keep = 1;

	len = strlen(t->txt);

	if(ctrlDown) {
		if (keysym == 'u' || keysym == 'U') {
			t->txt[0] = 0;
			t->offset = 0;
			t->next_clears = 0;
			RedrawTextEntryArea(t);
		}
	}
	else if(keysym == VK_DELETE || keysym == VK_BACK) {
		if (t->next_clears) {
			t->txt[0] = 0;
			t->offset = 0;
			len = 0;
			keep = 0;
		}
		if(len > 0) {
			t->txt[len - 1] = 0;
			if (len < t->offset + 2 && t->offset > 0) {
				t->offset -= t->cols - 3;
				if (t->offset < 0)
					t->offset = 0;
				keep = 0;
			}
			len--;
		}
		t->next_clears = 0;
		t->cursor = 1;
		if (keep) {
			SetColor(color[EMPTY][1]);
			XFillRectangle(t->x + FONTWIDTH * (len - t->offset) + 4, t->y + 3, FONTWIDTH + 3, HTEXTENTRIES - 6);
			SetColor(color[TXT][2]);
			XDrawLine(t->x + FONTWIDTH * (len - t->offset) + 6, t->y + 4, t->x + FONTWIDTH * (len - t->offset) + 6, t->y + 16);
			RestoreColors();
			XFlush();
		}
		else
		    RedrawTextEntryArea(t);
	}
	else if (keysym == VK_RETURN) {
		t->next_clears = 1;
		t->callback(t->txt, shiftDown);
		return;
	}
	else if ((keysym >= ' ' && keysym <= 0x7f) ||
		    (keysym >= 0xa1 && keysym <= 0xff)) {
		t->align = ALIGN_LEFT;
		if (t->next_clears) {
			t->txt[0] = 0;
			t->offset = 0;
			len = 0;
			keep = 0;
		}
		t->next_clears = 0;
		t->cursor = 1;
		if (len < t->maxlen) {
			t->txt[len] = keysym;
			t->txt[len + 1] = 0;
			if (len - t->offset >= t->cols - 1) {
				t->offset += t->cols - 3;
				keep = 0;
			}
			if (keep) {
				SetColor(color[EMPTY][1]);
				XFillRectangle(t->x + FONTWIDTH * (len - t->offset) + 4, t->y + 3, FONTWIDTH + 3, HTEXTENTRIES - 6);
				SetColor(color[TXT][2]);
				XDrawString(t->x + FONTWIDTH * (len - t->offset) + 4, t->y + FONTHEIGHT + 2, &t->txt[len], 1);
				XDrawLine(t->x + FONTWIDTH * (len - t->offset + 1) + 6, t->y + 4, t->x + FONTWIDTH * (len - t->offset + 1) + 6, t->y + 16);
				RestoreColors();
				XFlush();
			}
			else
			    RedrawTextEntryArea(t);
		}
	}
}


struct user *finduser(unsigned char c) {
	struct user *u = &me;

	for (; u; u=u->next)
		if (u->number == c)
			return u;

	fatal("Protocol error: reference to non-existing user");
	return NULL; /* so that gcc -Wall doesn't complain */
}

#define XFD (ConnectionNumber(display))

void doserverstuff(void);

#define tvgeq(a, b) ((a).tv_sec > (b).tv_sec ||  \
		      ((a).tv_sec == (b).tv_sec && (a).tv_usec >= (b).tv_usec))

#define tvnormal(a) do { \
		      while ((a).tv_usec >= 1000000) { \
		        (a).tv_usec -= 1000000;  \
			(a).tv_sec++;  \
		      }  \
		    } while(0)


void tvdiff(struct timeval *a, struct timeval *b, struct timeval *r) {
	if (a->tv_usec >= b->tv_usec) {
		r->tv_usec = a->tv_usec - b->tv_usec;
		r->tv_sec = a->tv_sec - b->tv_sec;
	}
	else {
		r->tv_usec = a->tv_usec + 1000000 - b->tv_usec;
		r->tv_sec = a->tv_sec - b->tv_sec - 1;
	}
}

void dofirsttimer(void) {
	struct timer *tm;

	tm = timer0;
	timer0 = timer0->next;
	tm->action(&tm->tv, tm->data);
	free(tm);
}

void addtimer(struct timeval *tv, v_vtp action, void *data) {
	struct timer *tm, *ltm;

	tm = mmalloc(sizeof(struct timer));
	tm->action = action;
	tm->data = data;
	tm->tv = *tv;
	if (!timer0 || tvgeq(timer0->tv, *tv)) {
		tm->next = timer0;
		timer0 = tm;
	}
	else {
		ltm = timer0;
		while (ltm->next && tvgeq(ltm->next->tv, *tv))
			ltm = ltm->next;
		tm->next = ltm->next;
		ltm->next = tm;
	}
}

// Processes any queued windows messages and if timeNow != NULL set's it to
// current time.
void processWinMessages(struct timeval *timeNow) {
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if(timeNow != NULL) {
		  gettimeofday(timeNow, NULL);
		}
	}
	if(needsRepaint) {
		redrawall();
		needsRepaint = FALSE;
	}
}

// We could divide gui thread from game thread...
/* wait while reading and responding to X events and reading server stuff */
void waitfor(int delay) {
	struct timeval tv, till, now;
	int r;
	fd_set rfds;

	interrupt = 0;
	gettimeofday(&now, NULL);
	till = now;
	till.tv_usec += delay;
	tvnormal(till);

	processWinMessages(&now);

	do {
		flushbuf();

		gettimeofday(&now, NULL);

		if (timer0) {
			while (timer0) {
				if (tvgeq(now, timer0->tv))
					dofirsttimer();
				else
				    break;
			}
		}

		if (tvgeq(now, till))
			break;
		else {
			//tvdiff(&till, &now, &tv);
			tv.tv_sec = 0;
			tv.tv_usec = 10000;
		}

		if (timer0 && tvgeq(till, timer0->tv))
			tvdiff(&timer0->tv, &now, &tv);

		FD_ZERO(&rfds);
		// In X Windows display has it's socket descriptor...
		//FD_SET(XFD, &rfds);
		if (!standalone)
			FD_SET(sfd, &rfds);

		//r = select(XFD > sfd ? XFD + 1 : sfd + 1, &rfds, NULL, NULL, &tv);
		r = select(sfd + 1, &rfds, NULL, NULL, &tv);
		if (r < 0 && errno != EINTR) {
			perror("select");
			fatal("fatal: select() failed");
		}
		if (r < 0)
			FD_ZERO(&rfds);

		//if (FD_ISSET(XFD, &rfds)) {
		//	while (XEventsQueued(display, QueuedAfterReading) > 0) {
		//		XNextEvent(display, &xev);
		//		doevent(&xev);
		//	}
		//}
		// Checking if there are any messages on queue was done without
		// FD_ISSET(XFD, &rfds)..
		processWinMessages(NULL);


		if (FD_ISSET(sfd, &rfds))
			doserverstuff();

		if (timer0) {
			while (timer0) {
				if (tvgeq(now, timer0->tv))
					dofirsttimer();
				else
				    break;
			}
		}
	}
	while (!interrupt);
}

void lostfallstep(struct timeval *tm, struct lostfalldata *lfd) {
	struct timeval ntm;
	int i;

	if (lfd->u->haslost) {
		memcpy(&oldpittmp[0][0], &(lfd->u->pit[0][0]), 200*sizeof(int));
		if (lfd->counter < 20) {
			for (i=0; i<10; i++)
				lfd->u->pit[19 - lfd->counter][i] = GRAY;
		}
		else {
			for (i=0; i<10; i++)
				lfd->u->pit[lfd->counter - 20][i] = EMPTY;
		}
		refreshuser(lfd->u);
		if (++lfd->counter >= 40)
			free(lfd);
		else {
			ntm = *tm;
			ntm.tv_usec += 50000;
			tvnormal(ntm);
			addtimer(&ntm, (v_vtp)lostfallstep, (void *)lfd);
		}
	}
	else free(lfd);
}


void do_replay(struct user *u) {
	if (nread + u->replaylen > BUFLEN)
		fatal("Internal error: read buffer overflow");
	if (u->replaylen) {
		memcpy(readbuf+nread, u->replaybuf, u->replaylen);
		nread += u->replaylen;
		u->replaylen = 0;
		handlemessages();
	}
}

void flashlinestep(struct timeval *tm, struct flashlinedata *fld) {
	struct timeval ntm;
	int i;

	if (fld->u->flashing) {
		fld->color = (fld->color+1) % 7;
		for (i=0; i<fld->nlines; i++)
			DrawLine(fld->u, fld->color, fld->lines[i]);
		XFlush();
		if (++fld->counter < 3) {
			ntm = *tm;
			ntm.tv_usec += 80000;
			tvnormal(ntm);
			addtimer(&ntm, (v_vtp)flashlinestep, (void *)fld);
		}
		else {
			memcpy(&oldpittmp[0][0], &fld->oldpit, 200*sizeof(int));
			fld->u->flashing = 0;
			refreshuser(fld->u);
			do_replay(fld->u);
			free(fld);
		}
	}
	else {
		memcpy(&oldpittmp[0][0], &fld->oldpit, 200*sizeof(int));
		fld->u->flashing = 0;
		refreshuser(fld->u);
		do_replay(fld->u);
		free(fld);
	}
}

void falldown(struct user *u) {
	struct lostfalldata *lfd;
	struct timeval tv;

	u->haslost = 1;
	lfd = mmalloc(sizeof(struct lostfalldata));
	lfd->u = u;
	lfd->counter = 0;
	gettimeofday(&tv, NULL);
	tv.tv_usec += 50000;
	tvnormal(tv);
	addtimer(&tv, (v_vtp)lostfallstep, (void *)lfd);
}

int checkstore(struct user *u, int len) {
	unsigned char *s;

	if (u->flashing) {
		if (u->replaylen + len + 4 >= BUFLEN)
			fatal("Internal error: replay buffer overflow");
		s = u->replaybuf + u->replaylen;
		*s++ = 0;
		*s++ = 0;
		*s++ = 0;
		*s++ = len;
		memcpy(s, buf, len);
		u->replaylen += len + 4;
		return 0;
	}
	else
	    return 1;
}

char isyou;

char *getid(unsigned char c) {
	static char rs[32];
	struct user *u = &me;

	sprintf(rs, "(unknown)(%d)", c);
	if (c == 0) {
		strcpy(rs, "the server");
	}
	else if (c == me.number) {
		strcpy(rs, "you");
		isyou = 1;
	}
	else {
		isyou = 0;
		for (; u; u=u->next)
			if (u->number == c)
				sprintf(rs, "client %d (%s)", u->number, u->nick);
	}
	return rs;
}

void handlemessages(void) {
	unsigned int len;
	struct user *u, *v = NULL;
	int i, j, x0 = -1;
	unsigned char *c, *s;
	struct flashlinedata *fld;
	struct timeval tv;
	static char msgbuf[300];

	while (nread >= 4 && nread >= 4 + readbuf[3]) {
		len = readbuf[3];
		if (readbuf[0] || readbuf[1] || readbuf[2])
			fatal("Wrong server line length");
		memcpy(buf, &readbuf[4], len);
		nread -= 4 + len;
		copydown(readbuf, readbuf + 4 + len, nread);

		switch(buf[1]) {
		case OP_YOUARE:
			me.number = buf[0];
			break;

		case OP_NEW:
			for (u=&me; u; u=u->next) {
				if (u->number == buf[0])
					fatal("fatal: duplicate player id");
				else {
					v = u;
					if (u->x0 > x0)
						x0 = u->x0;
				}
			}
			v->next = u = mmalloc(sizeof(struct user));
			memset(u, 0, sizeof(struct user));
			u->number = buf[0];
			u->games = (len >= 4 ? buf[2] * 256 + buf[3] : 0);
			u->nick[0] = 0;
			u->next = NULL;
			for (i=0; i<10; i++)
				for (j=0; j<20; j++)
					u->pit[j][i] = EMPTY;
			u->x0 = x0 + X_EACH;
			u->haslost = u->flashing = u->replaylen = 0;
			width += X_EACH;

			if (!textbutton)
				textbutton = DeclareButton(XTXT, YTXT, WTXT, HBTNS, 0, 1, "Text", ALIGN_CENTER, c_txt);

			scrollcols += WSCROLLRESIZE;
			SetTextScrollWidth(msgscroll, scrollcols);
			SetTextEntryWidth(msgentry, scrollcols);

			sprintf(msgbuf, "new client connected (%d)", u->number);
			sysmsg(msgbuf);

			UpdateWinSize();
			redrawall();
			break;

		case OP_GONE:
			u = finduser(buf[0]);
			sprintf(msgbuf, "%s disconnected", getid(buf[0]));
			sysmsg(msgbuf);

			if (checkstore(u, len)) {
				for (v=&me; v; v=v->next) {
					if (v->x0 > u->x0)
						v->x0 -= X_EACH;
					if (v->next == u)
						v->next = u->next;
				}
				width -= X_EACH;

				if (me.next == NULL && textbutton != NULL) {
					RemoveButton(textbutton);
					textbutton = NULL;
				}

				scrollcols -= WSCROLLRESIZE;
				SetTextScrollWidth(msgscroll, scrollcols);
				SetTextEntryWidth(msgentry, scrollcols);

				if (visiblemsgs && me.next == NULL) {
					height -= HSCROLLRESIZE;
					visiblemsgs = 0;
				}

				UpdateWinSize();

				free(u);
			}
			break;

		case OP_BADVERS:
			{
				static char tmpbuf[128];

				sprintf(tmpbuf, "Protocol version mismatch: server expects %d.%d.x instead of %d.%d.%d", buf[2], buf[3], PROT_VERS_1, PROT_VERS_2, PROT_VERS_3);
				fatal(tmpbuf);
			}
			break;

		case OP_PLAY:
			s = getid(buf[0]);
			sprintf(msgbuf, "%s start%s game", s, isyou ? "" : "s");
			sysmsg(msgbuf);
			got_play = interrupt = 1;
			break;

		case OP_PAUSE:
			s = getid(buf[0]);
			sprintf(msgbuf, "%s pause%s game", s, isyou ? "" : "s");
			sysmsg(msgbuf);
			paused = interrupt = 1;
			ChangeButtonAppearance(pausebutton, 1, NULL, -1);
			break;

		case OP_CONT:
			s = getid(buf[0]);
			sprintf(msgbuf, "%s continue%s game", s, isyou ? "" : "s");
			sysmsg(msgbuf);
			paused = 0;
			interrupt = 1;
			ChangeButtonAppearance(pausebutton, 0, NULL, -1);
			break;

      case OP_LINES:
	u = finduser(buf[0]);
	sprintf(msgbuf, "%s sends the wrong number of lines (ignored)", getid(buf[0]));
	if (funmode) {
	  switch(buf[2]) {
	    case 1:
	      addsquares++;
	      sprintf(msgbuf, "%s sends you one square", getid(buf[0]));
	      break;
	    case 2:
	      addlines++;
	      sprintf(msgbuf, "%s sends you one line", getid(buf[0]));
	      break;
	    case 3:
	      addlines += 3;
	      sprintf(msgbuf, "%s sends you 3 lines", getid(buf[0]));
	      break;
	    case 4:
	      sprintf(msgbuf, "%s sends you 5 lines", getid(buf[0]));
	      addlines += 5;
	      break;
	  }
	} else {
	  if (buf[2] >= 2 && buf[2] <= 4) {
	    sprintf(msgbuf, "%s sends you %d lines", getid(buf[0]), (int)buf[2]);
	    addlines += buf[2];
	  }
	}
	sysmsg(msgbuf);
	break;

      case OP_LINESTO:
	strcpy(msgbuf, getid(buf[0]));
	strcat(msgbuf, isyou ? " send " : " sends ");
	s = msgbuf + strlen(msgbuf);
	strcpy(s, "the wrong number of lines (ignored)");
	if (funmode) {
	  switch(buf[2]) {
	    case 1:
	      sprintf(s, "one square to %s", getid(buf[3]));
	      break;
	    case 2:
	      sprintf(s, "one line to %s", getid(buf[3]));
	      break;
	    case 3:
	      sprintf(s, "3 lines to %s", getid(buf[3]));
	      break;
	    case 4:
	      sprintf(s, "5 lines to %s", getid(buf[3]));
	      break;
	  }
	} else {
	  if (buf[2] >= 2 && buf[2] <= 4)
	    sprintf(s, "%d lines to %s", (int)buf[2], getid(buf[3]));
	}
	sysmsg(msgbuf);
	break;

		case OP_WON:
			u = finduser(buf[0]);
			sprintf(msgbuf, "%s %s the game", getid(buf[0]),
				isyou ? "win" : "wins");
			sysmsg(msgbuf);
			u->games++;
			updategames(u);
			break;

		case OP_ZERO:
			sprintf(msgbuf, "%s reset%s game counters", getid(buf[0]),
				isyou ? "" : "s");
			sysmsg(msgbuf);
			for (v=&me; v; v=v->next) {
				v->games = 0;
				updategames(v);
			}
			break;

		case OP_NICK:
			u = finduser(buf[0]);
			buf[len] = 0;
			sprintf(msgbuf, "%s calls itself %s", getid(buf[0]), &buf[2]);
			sysmsg(msgbuf);
			strncpy(u->nick, &buf[2], MAXNICKLEN);
			DrawButton(1, 1, XNICK + u->x0, YNICK, WNICK, HTEXTENTRIES, u->nick, ALIGN_CENTER, 0);
			XFlush();
			break;

		case OP_MSG:
			u = finduser(buf[0]);
			buf[len] = 0;
			msgfrom(u, &buf[2]);
			break;

		case OP_DRAW:
			u = finduser(buf[0]);
			if (checkstore(u, len)) {
				len -= 2;
				c = &buf[2];
				while (len >= 3) {
					DrawSqr(u, c[2], c[0], c[1]);
					u->pit[c[1]][c[0]] = c[2];
					c += 3;
					len -= 3;
				}
				if (len != 0)
					fatal("fatal: crap at the end of an OP_DRAW");
				XFlush();
			}
			break;

		case OP_FALL:
			if (len > 6 && len < 3)
				fatal("fatal: wrong OP_FALL command");
			u = finduser(buf[0]);

			if (checkstore(u, len)) {
				fld = mmalloc(sizeof(struct flashlinedata));
				memcpy(fld->lines, &buf[2], len-2);
				fld->counter = 0;
				fld->u = u;
				fld->nlines = len - 2;
				fld->color = i = my_rand() % 7;
				u->flashing = 1;
				u->lines += len - 2;
				updatelines(u);
				for (j=2; j<len; j++)
					DrawLine(u, i, buf[j]);
				XFlush();
				gettimeofday(&tv, NULL);
				tv.tv_usec += 80000;
				tvnormal(tv);
				addtimer(&tv, (v_vtp)flashlinestep, (void *)fld);

				memcpy(&fld->oldpit[0][0], &(u->pit[0][0]), 200*sizeof(int));
				for (j=2; j<len; j++) {
					falline(u, buf[j]);
					for (i=0; i<10; i++)
						fld->oldpit[buf[j]][i] |= 0x100;
				}
			}
			break;

		case OP_GROW:
			u = finduser(buf[0]);
			if (checkstore(u, len)) {
				memcpy(&oldpittmp[0][0], &(u->pit[0][0]), 200*sizeof(int));
				growline(u);
				refreshuser(u);
			}
			break;

		case OP_LOST:
			u = finduser(buf[0]);
			sprintf(msgbuf, "%s has lost", getid(buf[0]));
			sysmsg(msgbuf);
			if (checkstore(u, len)) {
				falldown(u);
			}
			break;

		case OP_CLEAR:
			u = finduser(buf[0]);
			if (checkstore(u, len)) {
				for (i=0; i<10; i++)
					for (j=0; j<20; j++)
						u->pit[j][i] = EMPTY;
				SetColor(color[EMPTY][1]);
				XFillRectangle(u->x0 + XPIT, YPIT, WPITIN, HPITIN);
				RestoreColors();
				XFlush();
			}
			break;

		case OP_LEVEL:
			s = getid(buf[0]);
			sprintf(msgbuf, "%s set%s level %d", s, isyou ? "" : "s", (int)buf[2]);
			sysmsg(msgbuf);
			setlevel(buf[2]);
			break;

		case OP_MODE:
			s = getid(buf[0]);
			sprintf(msgbuf, "%s set%s %s mode", s, isyou ? "" : "s", buf[2] ? "fun" : "normal");
			sysmsg(msgbuf);
			set_mode(buf[2]);
			break;
		}
	}
}

void doserverstuff(void) {
	int r;

	r = recv(sfd, readbuf + nread, BUFLEN - nread, 0);
	if (r < 0)
		fatal("Error reading from server");
	if (r == 0)
		fatal("Connection to server lost");
	nread += r;

	handlemessages();
}

void startserver(void) {
	char *options[4];
	int n = 0;
	int tmp;
	//VV 20040524 Port must be passed to server if we want to open server on
	//   non standard port...
	char portStr[16];

	options[0] = options[1] = options[2] = options[3] = NULL;
	sprintf(portStr, "%d", port);

	if (norestart)
		options[n++] = "-norestart";
	if (nospeedup)
		options[n++] = "-nospeedup";
	options[n++] = "-p";
	options[n++] = portStr;

	tmp = spawnlp(_P_NOWAIT,
	#ifdef XTRISPATH
		XTRISPATH "/xtserv.exe",
	#else
		"xtserv.exe",
	#endif
		"xtserv", "-once", options[0], options[1], options[2], options[3], NULL);

	if(tmp == -1) {
		fprintf(stderr, "Can't start server '%s'.\n",
	#ifdef XTRISPATH
		XTRISPATH "/xtserv.exe");
	#else
		"xtserv.exe");
	#endif
		status = -1;
		c_quit();
	}
}


void connect2server(char *h) {
	WSADATA wsaData;
	struct hostent *hp;
	struct sockaddr_in s4;
	struct sockaddr_in6 s6;
	struct sockaddr* s;
	int addr_size;
	struct protoent *tcpproto;
	int on = 1, i;
	unsigned long localhostAddr = inet_addr("127.0.0.1");

	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		fatal("Socket Initialization Error. Program aborted");
	}

	s = &s4;
	addr_size = sizeof(s4);
	memset(&s4, 0, sizeof(s4));
	s4.sin_family = AF_INET;
	s4.sin_port = htons(port);
	memset(&s6, 0, sizeof(s6));
	s6.sin6_family = AF_INET6;
	s6.sin6_port = htons(port);
	if (h) {
		if ((s4.sin_addr.s_addr = inet_addr(h)) == INADDR_NONE) {
			s = &s6;
			addr_size = sizeof(s6);
			if (inet_pton(AF_INET6, h, &s6.sin6_addr) != 1) {
				s = &s4;
				addr_size = sizeof(s4);
				hp = gethostbyname(h);
				if (!hp)
					fatal("Host not found");
				s4.sin_addr = *(struct in_addr *)(hp->h_addr_list[0]);
			}
		}
	}
	else {
		s4.sin_addr.s_addr = localhostAddr;
	}

	sfd = socket(s->sa_family, SOCK_STREAM, 0);
	if (sfd == INVALID_SOCKET)
		fatal("Out of file descriptors");
	if ((tcpproto = getprotobyname("tcp")) != NULL)
		setsockopt(sfd, tcpproto->p_proto, TCP_NODELAY, (char *)&on, sizeof(int));

	if (connect(sfd, s, addr_size) < 0) {
		if (!h || s4.sin_addr.s_addr == localhostAddr) {
			printf("No xtris server on localhost - starting up one ...\n");
			startserver();
			for (i=0; i<6; i++) {
				u_sleep(500000);
				closesocket(sfd);
				sfd = socket(s->sa_family, SOCK_STREAM, 0);
				if (sfd == INVALID_SOCKET)
					fatal("Out of file descriptors");
				setsockopt(sfd, tcpproto->p_proto, TCP_NODELAY, (char *)&on, sizeof(int));
				if (connect(sfd, s, addr_size) >= 0)
					break;
			}
			if (i==6)
				fatal("Can't connect to server");
		}
		else fatal("Can't connect to server");
	}
}


int main(int argc, char *argv[]) {
	char *s, *setnick = NULL;
	int i, j;
	struct toggle *t;
	// NEW
	char userName[USERNAME_SIZE];
	DWORD userNameSize = USERNAME_SIZE;

	while (argc >= 2) {
		if (strcmp(argv[1], "-bw") == 0) {
			argv++;
			argc--;
			bw = 1;
		}
		else if (strcmp(argv[1], "-h") == 0 ||
			    strcmp(argv[1], "-help") == 0) {
			printf(

				"Use: xtris [options] [server.name [port]]\n"
				    "Options:   -display machine:0   --  set the display\n"
				    "           -k keys              --  set keys (default is \"JjklL \")\n"
				    "           -n nick              --  sets the nickname of the player\n"
				    "           -s                   --  standalone mode\n"
				    "           -bw                  --  force black & white\n"
				    "           -big                 --  bigger squares\n"
				    "           -flashy              --  use flashier colors\n"
				    "           -norestart           --  pass a -norestart to the server\n"
				    "           -nospeedup           --  pass a -nospeedup to the server\n"
				    "           -v                   --  show xtris version\n"

				);

			exit(0);
		}
		else if (strcmp(argv[1], "-big") == 0) {
			sqrsize = 32;
			argv++;
			argc--;
		}
		else if (strcmp(argv[1], "-v") == 0) {
			printf("xtris version " VERSION "\n");
			exit(0);
		}
		else if (strcmp(argv[1], "-n") == 0) {
			if (argc < 3)
				fatal("Missing argument for -n");
			setnick = argv[2];
			argv += 2;
			argc -= 2;
		}
		else if (strcmp(argv[1], "-s") == 0) {
			standalone = 1;
			argv++;
			argc--;
		}
		else if (strcmp(argv[1], "-norestart") == 0) {
			norestart = 1;
			argv++;
			argc--;
		}
		else if (strcmp(argv[1], "-nospeedup") == 0) {
			nospeedup = 1;
			argv++;
			argc--;
		}
		else if (strcmp(argv[1], "-k") == 0) {
			if (argc < 3)
				fatal("Missing argument for -k");
			if (strlen(argv[2]) < 6)
				fatal("List of keys too short: must be 6 characters");
			for (i=0; i<6; i++)
				keys[i] = argv[2][i];
			keysspecified = 1;
			argv += 2;
			argc -= 2;
		}
		else if (strcmp(argv[1], "-flashy") == 0) {
			argv++;
			argc--;
			flashy = 1;
		}
		else if (argv[1][0] == '-') {
			fatal("Unrecognized option, try \"xtris -help\"");
		}
		else if (serverhost == NULL) {
			serverhost = argv[1];
			argv++;
			argc--;
		}
		else if (port != 0) {
			fatal("Too many arguments, try \"xtris -help\"");
		}
		else {
			port = atoi(argv[1]);
			if (port < 1024)
				fatal("Bad port number");
			argv++;
			argc--;
		}
	}

	me.number = 0;
	me.lines = 0;
	me.games = 0;
	me.x0 = LEFTSIDE;

	s = getenv("XTRISNAME");
	if(s ==  NULL) {
		s = getenv("NETTRISNAME");
	}
	if(s != NULL) {
		strncpy(me.nick, s, MAXNICKLEN);
	}
	else if(GetUserName(userName, &userNameSize) == TRUE) {
		strncpy(me.nick, userName, USERNAME_SIZE);
	}
	else {
	    strcpy(me.nick, "unknown");
	}

	me.next = NULL;

	if (port == 0)
		port = DEFAULTPORT;

	if (!standalone)
		connect2server(serverhost); /* possibly NULL */

	scrollcols = WSCROLL - WSCROLLRESIZE;

	init_X();

	if (setnick != NULL)
		strncpy(me.nick, setnick, MAXNICKLEN);

	quitbutton = DeclareButton(XBTNS, YQUIT, WBTNS, HBTNS, 0, 1, "Quit", ALIGN_CENTER, c_quit);
	playbutton = DeclareButton(XBTNS, YPLAY, WBTNS, HBTNS, 0, 1, "Start", ALIGN_CENTER, c_play);
	pausebutton = DeclareButton(XBTNS, YPAUSE, WBTNS, HBTNS, 0, 0, "Pause", ALIGN_CENTER, c_pause);
	modebutton = DeclareButton(XBTNS, YMODE, WBTNS, HBTNS, 0, 1, "Normal", ALIGN_CENTER, c_mode);
	verbosebutton = DeclareButton(XBTNS, YVERBOSE, WBTNS, HBTNS, verbose, 0, "Verbose", ALIGN_CENTER, c_verbose);
	clearbutton = DeclareButton(XBTNS, YCLEAR, WBTNS, HBTNS, 0, 1, "Clear", ALIGN_CENTER, c_clear);

	if (!standalone) {
		botbutton = DeclareButton(XBTNS, YBOT, WBTNS, HBTNS, 0, 1, "Bot", ALIGN_CENTER, c_bot);
	}

	t = leveltogs[5] = DeclareToggle(XLVLS+26, YLVLS+26, WL, HL, "5", ALIGN_CENTER, c_speed5, NULL);
	leveltogs[4] = DeclareToggle(XLVLS, YLVLS+26, WL, HL, "4", ALIGN_CENTER, c_speed4, t);
	leveltogs[6] = DeclareToggle(XLVLS+52, YLVLS+26, WL, HL, "6", ALIGN_CENTER, c_speed6, t);
	leveltogs[1] = DeclareToggle(XLVLS, YLVLS, WL, HL, "1", ALIGN_CENTER, c_speed1, t);
	leveltogs[2] = DeclareToggle(XLVLS+26, YLVLS, WL, HL, "2", ALIGN_CENTER, c_speed2, t);
	leveltogs[3] = DeclareToggle(XLVLS+52, YLVLS, WL, HL, "3", ALIGN_CENTER, c_speed3, t);
	leveltogs[7] = DeclareToggle(XLVLS, YLVLS+52, WL, HL, "7", ALIGN_CENTER, c_speed7, t);
	leveltogs[8] = DeclareToggle(XLVLS+26, YLVLS+52, WL, HL, "8", ALIGN_CENTER, c_speed8, t);
	leveltogs[9] = DeclareToggle(XLVLS+52, YLVLS+52, WL, HL, "9", ALIGN_CENTER, c_speed9, t);

	msgentry = DeclareTextEntry(XMSGENTRY, YMSGENTRY, scrollcols, 240, "", 1, c_msgentry);

	msgscroll = DeclareTextScroll(XSCROLL, YSCROLL, WSCROLL, HSCROLL, color[TXT][2], msgentry);

	zerobutton = DeclareButton(XZERO, YZERO, WZERO, HBTNS, 0, 1, "Zero", ALIGN_CENTER, c_zero);

	nickentry = DeclareTextEntry(XNICK + me.x0, YNICK, WNICKCHARS, MAXNICKLEN, "", 0, c_setnick);
	SimulateTextEntry(nickentry, me.nick);

	x_initted = 1;

	buf[1] = OP_VERSION;
	buf[2] = PROT_VERS_1;
	buf[3] = PROT_VERS_2;
	buf[4] = PROT_VERS_3;
	sendbuf(5);

	DrawFrame();
	for (i=0; i<20; i++)
		for (j=0; j<10; j++)
			me.pit[i][j] = oldpit[i][j] = EMPTY;
	XFlush();

	runGame();
	return 0;
}

void runGame(void) {
	int i, j;
	struct user *u;

	redrawall();
	while(1) {
		playing = 0;
		do {
			waitfor(100000);
		}
		while(!got_play);

start_game:
		buf[1] = OP_CLEAR;
		sendbuf(2);
		got_play = 0;
		addlines = 0;
		addsquares = 0;
		playing = 1;
		for (u=&me; u; u=u->next) {
			u->haslost = 0;
			u->flashing = 0;
			u->replaylen = 0;
			u->lines = 0;
			updatelines(u);
		}
		for (i=0; i<20; i++)
			for (j=0; j<10; j++)
				me.pit[i][j] = oldpit[i][j] = EMPTY;
		SetColor(color[EMPTY][1]);
		XFillRectangle(me.x0 + XPIT, YPIT, WPITIN, HPITIN);
		RestoreColors();
		clearnext();

		if (funmode) {
			for (i=10; i<20; i++) {
				me.pit[i][0] = my_rand() % 7;
				me.pit[i][9] = my_rand() % 7;
			}
			for (i=0; i<24; i++)
				me.pit[15 + my_rand() % 5][my_rand() % 10] = my_rand() % 7;
			refresh();
		}

		XFlush();

		next = my_rand() % 7;
		drawnext();
		while (fits(piece = next, rotation = 0, x = 2, y = 3)) {
			clearnext();
			next = my_rand() % 7;
			drawnext();
			while(fits(piece, rotation, x, y)) {
				put(piece, rotation, x, y, piece);
				refresh();
				XFlush();
				while (paused)
					waitfor(100000);
				waitfor(delays[level-1]);
				while (paused)
					waitfor(100000);
				if (got_play)
					goto start_game;
				remove(piece, rotation, x, y);
				y++;
			}
			put(piece, rotation, x, --y, piece);
			if (sticksout(piece, rotation, x, y))
				break;
			fallines();
			if (!newlines() || !newsquares())
				break;
		}
		buf[1] = OP_LOST;
		sendbuf(2);
		sysmsg("you have lost");
		if (standalone) {
			me.games++;
			updategames(&me);
		}
		clearnext();
		falldown(&me);
	}
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
    case WM_ACTIVATE:
//        // If window is activated and is not minimized.
//        if(LOWORD(wParam) == WA_INACTIVE && HIWORD(wParam) == 0) {
          redrawall();
//        }
        break;
    case WM_WINDOWPOSCHANGED:
				redrawall();
				break;
		case WM_KEYDOWN:
			if(wParam == VK_CONTROL) {
				ctrlDown = TRUE;
			}
			else if(wParam == VK_SHIFT) {
				shiftDown = TRUE;
			}
			else {
				processKeyEvent(wParam);
			}
			break;
		case WM_KEYUP:
			if(wParam == VK_CONTROL) {
				ctrlDown = FALSE;
			}
			else if(wParam == VK_SHIFT) {
				shiftDown = FALSE;
			}
			break;

		case WM_LBUTTONDOWN:
			xLastPressed = LOWORD(lParam);
			yLastPressed = HIWORD(lParam);
			break;
		case WM_LBUTTONUP:
		{
			int px = LOWORD(lParam);
			int py = HIWORD(lParam);
			// Check for mouse click (last pressed on same place as this released.
			if(abs(xLastPressed - px) <= 0 && abs(yLastPressed - py) <= 0) {
				struct button *b;
				struct toggle *t;
				struct user *u;
				for (b=button0; b; b=b->next) {
					if (px >= b->x && px <= b->x + b->w && py >= b->y &&
						    py <= b->y + b->h) {
						SimulateButtonPress(b);
						break;
					}
				}
				for (t=tog0; t; t=t->next) {
					if (px >= t->x && px <= t->x + t->w && py >= t->y &&
						    py <= t->y + t->h)  {
						if (!t->down) {
							t->callback();
							ToggleDown(t);
						}
						break;
					}
				}
				for (u=me.next; u; u=u->next) {
					if (px >= u->x0 + XNICK && px <= u->x0 + XNICK + WNICK &&
						    py >= YNICK && py <= YNICK + HTEXTENTRIES) {
						buf[1] = OP_KILL;
						buf[2] = u->number;
						sendbuf(3);
						break;
					}
				}
				if (px >= XNEXT && px <= XNEXT + WNEXT && py >= YNEXT &&
					    py <= YNEXT + HNEXT && playing) {
					if (shownext) {
						clearnext();
						shownext = 0;
					}
					else {
						shownext = 1;
						drawnext();
					}
				}

			}
		}
			break;


		case WM_CLOSE:
	      c_quit();
	    	break;
	  case WM_DESTROY:
	      PostQuitMessage(0);
	      c_quit();
	    	break;

    case WM_ERASEBKGND:
        needsRepaint = TRUE;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
	}
  return 0;
}

void processKeyEvent(int vkCode) {
	POINT p;
	struct textentry *te;
	struct textscroll *ts;
	int i;
	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);


	if (!x_initted)
		return;
	if(!shiftDown) {
		vkCode = tolower(vkCode);
	}

	/* this one goes before the text entry areas! */
	if (vkCode == warp_key) {
		if (warp_next == 1 && !visiblemsgs)
			warp_next = 2;
		switch (warp_next) {
		case 0:
			XWarpPointer(3, 3);
			break;
		case 1:
			XWarpPointer(XSCROLL + scrollcols * FONTWIDTH, YMSGENTRY + HTEXTENTRIES - 3);
			break;
		case 2:
			XWarpPointer(XNICK + me.x0 + WNICKCHARS * FONTWIDTH, YNICK + HTEXTENTRIES - 3);
			break;
		}
		warp_next = (warp_next + 1) % 3;
		return;
	}

	for (te=text0; te; te=te->next) {
		if (p.x >= te->x && p.x <= te->x + te->w &&
			    p.y >= te->y && p.y <= te->y + te->h) {
			textentrykey(te, vkCode);
			return;
		}
	}
	for (ts=scroll0; ts; ts=ts->next) {
		if (ts->redirect &&
			    p.x >= ts->x && p.x <= ts->x + ts->w &&
			    p.y >= ts->y && p.y <= ts->y + ts->h) {
			textentrykey(ts->redirect, vkCode);
			return;
		}
	}

	if (vkCode == quit_key) {
		SimulateButtonPress(quitbutton);
	}

	if (vkCode == start_key) {
		SimulateButtonPress(playbutton);
		return;
	}

	if (vkCode == pause_key) {
		SimulateButtonPress(pausebutton);
		return;
	}

	if (vkCode == bot_key && botbutton != NULL) {
		SimulateButtonPress(botbutton);
		return;
	}

	if (vkCode == zero_key) {
		SimulateButtonPress(zerobutton);
		return;
	}

	if (vkCode == text_key && textbutton != NULL) {
		SimulateButtonPress(textbutton);
		return;
	}

	if (vkCode == mode_key) {
		SimulateButtonPress(modebutton);
		return;
	}

	if (visiblemsgs && vkCode == verbose_key) {
		SimulateButtonPress(verbosebutton);
		return;
	}

	if (visiblemsgs && vkCode == clear_key) {
		SimulateButtonPress(clearbutton);
		return;
	}

	for (i=0; i<10; i++) {
		if (vkCode == level_keys[i]) {
			sendlevel(i);
			return;
		}
	}

	if (!playing || paused)
		return;

	if (vkCode == keys[5] ||
		    (vkCode == VK_DOWN && shiftDown)) {
		remove(piece, rotation, x, y);
		while(fits(piece, rotation, x, ++y));
		y--;
		put(piece, rotation, x, y, piece);
		refresh();
		XFlush();
	}
	else if (vkCode == keys[1] ||
		    (vkCode == VK_LEFT && !shiftDown)) {
		remove(piece, rotation, x, y);
		if (fits(piece, rotation, --x, y)) {
			put(piece, rotation, x, y, piece);
			refresh();
			XFlush();
		}
		else put(piece, rotation, ++x, y, piece);
	}
	else if (vkCode == keys[3] ||
		    (vkCode == VK_RIGHT && !shiftDown)) {
		remove(piece, rotation, x, y);
		if (fits(piece, rotation, ++x, y)) {
			put(piece, rotation, x, y, piece);
			refresh();
			XFlush();
		}
		else put(piece, rotation, --x, y, piece);
	}
	else if (vkCode == keys[2] ||
		    ((keys[2]&0xdf) >= 'A' && (keys[2]&0xdf) <= 'Z' &&
		    vkCode == (keys[2]^0x20)) ||
		    (vkCode == VK_UP)) {
		remove(piece, rotation, x, y);
		rotation = (rotation + 1) & 3;
		if (fits(piece, rotation, x, y)) {
			put(piece, rotation, x, y, piece);
			refresh();
			XFlush();
		}
		else {
			rotation = (rotation + 3) & 3;
			put(piece, rotation, x, y, piece);
		}
	}
	else if (vkCode == keys[0] ||
		    (vkCode == VK_LEFT && shiftDown)) {
		remove(piece, rotation, x, y);
		while (fits(piece, rotation, --x, y));
		x++;
		put(piece, rotation, x, y, piece);
		refresh();
		XFlush();
	}
	else if (vkCode == keys[4] ||
		    (vkCode == VK_RIGHT && shiftDown)) {
		remove(piece, rotation, x, y);
		while (fits(piece, rotation, ++x, y));
		x--;
		put(piece, rotation, x, y, piece);
		refresh();
		XFlush();
	}
	else if (vkCode == down_key ||
		    (vkCode == VK_DOWN && !shiftDown)) {
		remove(piece, rotation, x, y);
		if (fits(piece, rotation, x, ++y)) {
			put(piece, rotation, x, y, piece);
			refresh();
			XFlush();
		}
		else put(piece, rotation, x, --y, piece);
	}
	else if (vkCode == togglenext_key) {
		if (shownext) {
			clearnext();
			shownext = 0;
		}
		else {
			shownext = 1;
			drawnext();
		}
	}
}
