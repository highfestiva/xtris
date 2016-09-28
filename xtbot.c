/*
 *   An automatic player for xtris, a multi-player version of Tetris.
 *
 *   Copyright (C) 1996 Roger Espel Llima <roger.espel.llima@pobox.com>
 *
 *   Started: 10 Oct 1996
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


/*
*   This is only the skeleton for the bot; all the 'AI' stuff is done
*   in decide.c.  To make your own bots, you should be able to replace
*   just the part in decide.c and link them with xtbot.o.  See the
*   comments in decide.c, xtbot.h and decide.h for more information.
*/

#pragma comment(lib, "Ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "systime.h"
#include <winsock2.h>
#include <errno.h>
#include <process.h>
#include "xtbot.h"
#include "decide.h"

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



int shape[7][4][8] = {

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
int rotations[7] = { 2, 1, 4, 4, 4, 2, 2 };

int pit[20][10], oldpit[20][10];


static int funmode = 0;
static int paused = 0;
static int quiet = 0;

static int testmode = 0, nlines = 0, nbricks = 0, totallines = 0;
static int totalbricks = 0, maxlines = 0, maxbricks = 0;

/* server stuff */

#define BUFLEN 4096

static unsigned int sfd;
static int port = DEFAULTPORT;
static unsigned char realbuf[512], readbuf[BUFLEN], writbuf[BUFLEN];
static unsigned char *buf = realbuf + 4;
static int nread = 0, nwrite = 0;

static char mynick[16];

static char simkeys[200], *sk = NULL;

static int delays[10] = { 1500000, 1200000, 900000, 700000, 500000, 350000,
	200000, 120000, 80000 };
static int piece, rotation, x, y, next, level = 5;
static int playing = 0, got_play = 0, interrupt = 0, addlines = 0;
static int addsquares = 0;

static char keys[] = "JjklL ";

/* like memcpy, but guaranteed to handle overlap when s <= t */
void copydown(char *s, char *t, int n) {
	for (; n; n--)
		*(s++) = *(t++);
}

/* same, in case s >= t; s and t are starting positions */
void copyup(char *s, char *t, int n) {
	t += n-1;
	s += n-1;
	for (; n; n--)
		*(s--) = *(t--);
}

void fatal(char *s) {
	if (!quiet)
		fprintf(stderr, "%s.\n", s);
	WSACleanup();
	exit(1);
}

void flushbuf(void) {
	if (nwrite) {
		send(sfd, writbuf, nwrite, 0);
		nwrite = 0;
	}
}

static void sendbuf(int len) {
	realbuf[0] = realbuf[1] = realbuf[2] = 0;
	realbuf[3] = (unsigned char)len;
	buf[0] = 0;
	if (nwrite + 4 + len >= BUFLEN)
		fatal("Internal error: send buffer overflow");
	memcpy(writbuf + nwrite, realbuf, 4 + len);
	nwrite += 4 + len;
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
		if (yy >= 0 && pit[yy][xx] != EMPTY)
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
			pit[yy][xx] = color;
	}
}

static void refresh(void) {
	int x, y;
	int i = 2;

	buf[1] = OP_DRAW;

	for (x=0; x<10; x++) {
		for (y=0; y<20; y++) {
			if (pit[y][x] != oldpit[y][x]) {
				buf[i] = x;
				buf[i+1] = y;
				buf[i+2] = pit[y][x];
				i += 3;
				if (i > 200) {
					sendbuf(i);
					i = 2;
					buf[1] = OP_DRAW;
				}
				oldpit[y][x] = pit[y][x];
			}
		}
	}
	if (i > 2)
		sendbuf(i);
}

static void refreshme(void) {
	int x, y;

	for (x=0; x<10; x++)
		for (y=0; y<20; y++)
			if (pit[y][x] != oldpit[y][x])
				oldpit[y][x] = pit[y][x];
}

static void copyline(int i, int j) {
	int n;

	for (n=0; n<10; n++)
		pit[j][n] = pit[i][n];
}

static void emptyline(int i) {
	int n;

	for (n=0; n<10; n++)
		pit[i][n] = EMPTY;
}

static void falline(int i) {
	int x;
	for (x = i-1; x>=0; x--)
		copyline(x, x+1);
	emptyline(0);
}

static void growline(void) {
	int n;
	for (n=0; n<19; n++)
		copyline(n+1, n);
	emptyline(19);
}

static void fallines(void) {
	int n, c, x, fallen = 0;

	buf[1] = OP_FALL;
	for (n=0; n<20; n++) {
		c = 0;
		for (x=0; x<10; x++)
			if (pit[n][x] != EMPTY)
				c++;
		if (c == 10) {
			falline(n);
			buf[2 + fallen++] = (unsigned char)n;
			totallines++;
			nlines++;
		}
	}
	if (fallen && !testmode) {
		sendbuf(2 + fallen);
		refreshme();
	}
	if ((fallen > 1 || (fallen > 0 && funmode)) && !testmode) {
		buf[1] = OP_LINES;
		buf[2] = (unsigned char)fallen;
		sendbuf(3);
	}
}

static int newlines(void) {
	int i = addlines, j, k, r = 1;

	if (playing) {
		while (i > 0) {
			for (j=0; j<10; j++)
				if (pit[0][j] != EMPTY)
					r = 0;
			growline();
			buf[1] = OP_GROW;
			sendbuf(2);
			k = 0;
			for (j=0; j<10; j++)
				if (my_rand() % 7 < 3) {
					pit[19][j] = my_rand() % 7;
					k++;
				}
			if (k == 10)
				pit[19][my_rand() % 10] = EMPTY;
			i--;
		}
		if (addlines > 0) {
			refreshme();
			for (j=20-addlines; j<20; j++)
				for (k=0; k<10; k++)
					oldpit[j][k] = EMPTY;
			refresh();
		}
		addlines = 0;
	}
	return r;
}

static int newsquares(void) {
	int n = addsquares, i, j, k, r = 1;

	if (playing) {
		while (n > 0) {
			for (i=0; i<10; i++)
				if (pit[0][i] != EMPTY)
					r = 0;
			for (i=0; i<20; i++) {
				k = 0;
				for (j=0; j<10; j++) {
					if (pit[i][j] != EMPTY)
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
					if (pit[i][k] != EMPTY)
						if (j == 0) {
							pit[i-1][k] = GRAY;
							break;
						}
						else {
							j--;
						}
				}
			}
			n--;
		}
		if (addsquares > 0)
			refresh();
		addsquares = 0;
	}
	return r;
}

static void setlevel(int n) {
	if (n < 1)
		n = 1;
	if (n > 9)
		n = 9;
	level = n;
}

static void set_mode(int n) {
	funmode = (n != 0);
}

static void dokey(char keysym) {
	if (!playing)
		return;

	if (keysym == keys[5]) {
		remove(piece, rotation, x, y);
		while(fits(piece, rotation, x, ++y));
		y--;
		put(piece, rotation, x, y, piece);
		refresh();
	}
	else if (keysym == keys[1]) {
		remove(piece, rotation, x, y);
		if (fits(piece, rotation, --x, y)) {
			put(piece, rotation, x, y, piece);
			refresh();
		}
		else put(piece, rotation, ++x, y, piece);
	}
	else if (keysym == keys[3]) {
		remove(piece, rotation, x, y);
		if (fits(piece, rotation, ++x, y)) {
			put(piece, rotation, x, y, piece);
			refresh();
		}
		else put(piece, rotation, --x, y, piece);
	}
	else if (keysym == keys[2]) {
		remove(piece, rotation, x, y);
		rotation = (rotation + 1) & 3;
		if (fits(piece, rotation, x, y)) {
			put(piece, rotation, x, y, piece);
			refresh();
		}
		else {
			rotation = (rotation + 3) & 3;
			put(piece, rotation, x, y, piece);
		}
	}
	else if (keysym == keys[0]) {
		remove(piece, rotation, x, y);
		while (fits(piece, rotation, --x, y));
		x++;
		put(piece, rotation, x, y, piece);
		refresh();
	}
	else if (keysym == keys[4]) {
		remove(piece, rotation, x, y);
		while (fits(piece, rotation, ++x, y));
		x--;
		put(piece, rotation, x, y, piece);
		refresh();
	}
}

static void doserverstuff(void);

#define tvgeq(a, b) ((a).tv_sec > (b).tv_sec ||  \
		      ((a).tv_sec == (b).tv_sec && (a).tv_usec >= (b).tv_usec))

#define tvnormal(a) do { \
		      while ((a).tv_usec >= 1000000) { \
		        (a).tv_usec -= 1000000;  \
			(a).tv_sec++;  \
		      }  \
		    } while(0)


static void tvdiff(struct timeval *a, struct timeval *b, struct timeval *r) {
	if (a->tv_usec >= b->tv_usec) {
		r->tv_usec = a->tv_usec - b->tv_usec;
		r->tv_sec = a->tv_sec - b->tv_sec;
	}
	else {
		r->tv_usec = a->tv_usec + 1000000 - b->tv_usec;
		r->tv_sec = a->tv_sec - b->tv_sec - 1;
	}
}

static void waitfor(int delay) {
	/* wait while reading and responding to server stuff */

	struct timeval tv, till, now;
	int r;
	fd_set rfds;

	interrupt = 0;
	gettimeofday(&now, NULL);
	till = now;
	till.tv_usec += delay;
	tvnormal(till);

	do {
		flushbuf();

		FD_ZERO(&rfds);
		FD_SET(sfd, &rfds);

		gettimeofday(&now, NULL);

		if (tvgeq(now, till))
			break;
		else
		    tvdiff(&till, &now, &tv);

		r = select(sfd + 1, &rfds, NULL, NULL, &tv);
		if (r < 0 && errno != EINTR) {
			perror("select");
			fatal("fatal: select() failed");
		}
		if (r < 0)
			FD_ZERO(&rfds);

		if (FD_ISSET(sfd, &rfds))
			doserverstuff();

	}
	while (!interrupt);
}

static void doserverstuff() {
	int r;
	unsigned int len;

	//r = read(sfd, readbuf + nread, 1024 - nread);
	r = recv(sfd, readbuf + nread, 1024 - nread, 0);
	if(r <0)
		fatal("Error reading from server");
	if(r == 0)
		fatal("Connection to server lost");
	nread += r;

	while (nread >= 4 && nread >= 4 + readbuf[3]) {
		len = readbuf[3];
		if (readbuf[0] || readbuf[1] || readbuf[2])
			fatal("Wrong server line length");
		memcpy(buf, &readbuf[4], len);
		nread -= 4 + len;
		copydown(readbuf, readbuf + 4 + len, nread);

		switch(buf[1]) {
		case OP_PLAY:
			got_play = interrupt = 1;
			break;

		case OP_PAUSE:
			paused = interrupt = 1;
			break;

		case OP_CONT:
			paused = 0;
			interrupt = 1;
			break;

		case OP_LINES:
			if (funmode) {
				switch(buf[2]) {
				case 1:
					addsquares++;
					break;
				case 2:
					addlines++;
					break;
				case 3:
					addlines += 3;
					break;
				case 4:
					addlines += 5;
					break;
				}
			}
			else {
				if (buf[2] >= 2 && buf[2] <= 4)
					addlines += buf[2];
			}
			break;

		case OP_LEVEL:
			setlevel(buf[2]);
			break;

		case OP_MODE:
			set_mode(buf[2]);
			break;

		case OP_BADVERS:
			{
				static char tmpbuf[128];

				sprintf(tmpbuf, "Protocol version mismatch: server expects %d.%d.x instead of %d.%d.%d", buf[2], buf[3], PROT_VERS_1, PROT_VERS_2, PROT_VERS_3);
				fatal(tmpbuf);
			}
			break;

		}
	}
}

static void connect2server(char *h) {
	WSADATA wsaData;
	struct hostent *hp;
	struct sockaddr_in s;
	struct protoent *tcpproto;
	int on = 1;

	if (h) {
		if ((s.sin_addr.s_addr = inet_addr(h)) == INADDR_NONE) {
			hp = gethostbyname(h);
			if (!hp)
				fatal("Host not found");
			s.sin_addr = *(struct in_addr *)(hp->h_addr_list[0]);
		}
	}
	else s.sin_addr.s_addr = inet_addr("127.0.0.1");
	s.sin_port = htons(port);
	s.sin_family = AF_INET;

	if(WSAStartup(MAKEWORD(2,0),&wsaData) != 0) {
		fatal("Socket Initialization Error. Program aborted");
	}

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == INVALID_SOCKET)
		fatal("Out of file descriptors");
	if ((tcpproto = getprotobyname("tcp")) != NULL)
		setsockopt(sfd, tcpproto->p_proto, TCP_NODELAY, (char *)&on, sizeof(int));

	if (connect(sfd, (struct sockaddr *)&s, sizeof(s)) < 0)
		fatal("Can't connect to server");

	buf[1] = OP_NICK;
	memcpy(&buf[2], mynick, strlen(mynick));
	sendbuf(2 + strlen(mynick));
	buf[1] = OP_BOT;
	sendbuf(2);
	buf[1] = OP_VERSION;
	buf[2] = PROT_VERS_1;
	buf[3] = PROT_VERS_2;
	buf[4] = PROT_VERS_3;
	sendbuf(5);
}

static void make_simkeys(int x0, int x, int rot) {
	int maxheight = 19;
	int addstars, len;
	int i, j;

	simkeys[0] = 0;
	sk = simkeys;
	for (i=0; i<19 && maxheight == 19; i++) {
		for (j=0; j<10 ; j++) {
			if (pit[i][j] != EMPTY && maxheight > i) {
				maxheight = i;
				break;
			}
		}
	}

	for (i=0; i<rot; i++)
		*sk++ = 'k';

	if (x > x0)
		for (i=x0; i<x; i++)
			*sk++ = 'l';
	else
		for (i=x0; i>x; i--)
			*sk++ = 'j';

	*sk = 0;
	len = strlen(simkeys);

	if (len) {

		addstars = maxheight - 4;
		if (addstars >= len)
			addstars = len - 1;
		if (addstars < 0)
			addstars = 0;

		sk = simkeys;
		for (; addstars; addstars--) {
			i = 1 + (my_rand() % (3 * len));
			for (; i; i--) {
				sk++;
				if (*sk == 0)
					sk = simkeys;
				while (*sk == '*' || sk[1] == '*' || sk[1] == 0) {
					sk++;
					if (*sk == 0)
						sk = simkeys;
				}
			}
			copyup(sk+2, sk+1, 2*len);
			sk[1] = '*';
		}
	}

	strcat(simkeys, "** ");
	sk = simkeys;
}

static void dotest(void) {
	int i, j;

	addlines = 0;
	nlines = nbricks = 0;
	addsquares = 0;
	playing = 1;

	for (i=0; i<20; i++)
		for (j=0; j<10; j++)
			pit[i][j] = EMPTY;

	while (fits(piece = next, rotation = 0, x = 2, y = 3)) {
		totalbricks++;
		nbricks++;
		next = my_rand() % 7;
		decide(piece, y, -3, 6, &x, &y, &rotation);
		put(piece, rotation, x, y, piece);
		if (sticksout(piece, rotation, x, y))
			break;
		fallines();
	}
	if (nlines > maxlines)
		maxlines = nlines;
	if (nbricks > maxbricks)
		maxbricks = nbricks;
}

int main(int argc, char *argv[]) {
	int i, j, newx, newy, newrot;

	strcpy(mynick, "xtris bot");

	init_decide();

	while (argc >= 2 && argv[1][0] == '-') {
		if (strcmp(argv[1], "-h") == 0 ||
			    strcmp(argv[1], "-help") == 0) {
			printf("Use: xtbot [ -n nick ] [ -quiet ] [ -test ] [ server.name [ port ] ]\n");
			exit(0);
		}
		else if (strcmp(argv[1], "-quiet") == 0) {
			quiet = 1;
			argv++;
			argc--;
		}
		else if (strcmp(argv[1], "-test") == 0) {
			testmode = 1;
			argv++;
			argc--;
		}
		else if (strcmp(argv[1], "-n") == 0) {
			if (argc < 3)
				fatal("Missing argument for -n");
			strncpy(mynick, argv[2], 14);
			argv += 2;
			argc -= 2;
		}
		else fatal("Unrecognized option, try \"xtbot -help\"");
	}

	if (argc > 2) {
		port = atoi(argv[2]);
		if (port < 1024)
			fatal("bad port number");
	}

	if (testmode) {
		for (i=0; i<16; i++) {
			dotest();
			printf("lines: %d, bricks %d\n", nlines, nbricks);
		}
		printf("\naverage lines: %d, average bricks: %d\n", (totallines+8)/16,
			(totalbricks + 8)/16);
		printf("max lines: %d, max bricks: %d\n", maxlines, maxbricks);
		exit(0);
	}

	connect2server(argv[1]);

	for (i=0; i<20; i++)
		for (j=0; j<10; j++)
			pit[i][j] = oldpit[i][j] = EMPTY;

	while(1) {
		playing = 0;
		do {
			waitfor(1000000);
		}
		while(!got_play);

start_game:
		buf[1] = OP_CLEAR;
		sendbuf(2);
		got_play = 0;
		addlines = 0;
		addsquares = 0;
		playing = 1;

		for (i=0; i<20; i++)
			for (j=0; j<10; j++)
				pit[i][j] = oldpit[i][j] = EMPTY;

		if (funmode) {
			for (i=10; i<20; i++) {
				pit[i][0] = my_rand() % 7;
				pit[i][9] = my_rand() % 7;
			}
			for (i=0; i<24; i++)
				pit[15 + my_rand() % 5][my_rand() % 10] = my_rand() % 7;
			refresh();
		}

		next = my_rand() % 7;
		while (fits(piece = next, rotation = 0, x = 2, y = 3)) {
			next = my_rand() % 7;
			decide(piece, y, -3, 6, &newx, &newy, &newrot);
			make_simkeys(x, newx, newrot);
			while(fits(piece, rotation, x, y)) {
				put(piece, rotation, x, y, piece);
				refresh();
				if (got_play)
					goto start_game;
				if (sk && *sk) {
					while (*sk && *sk != '*')
						dokey(*sk++);
					if (*sk == '*')
						sk++;
				}
				while (paused)
					waitfor(1000000);
				waitfor(delays[level-1]);
				while (paused)
					waitfor(1000000);
				remove(piece, rotation, x, y);
				y++;
			}
			y--;
			put(piece, rotation, x, y, piece);
			if (sticksout(piece, rotation, x, y))
				break;
			fallines();
			if (!newlines() || !newsquares())
				break;
		}
		buf[1] = OP_LOST;
		sendbuf(2);
	}
}


