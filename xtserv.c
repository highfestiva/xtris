/*
 *   A server for a multi-player version of Tetris
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
 
#pragma comment(lib, "Ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <winsock2.h>
#include <errno.h>
#include <string.h>
#include <process.h>

#define DEFAULTPORT 19503

#define PROT_VERS_1 1
#define PROT_VERS_2 0
#define PROT_VERS_3 1

#define RESTARTDELAY 4

#define OP_NICK	1
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

void do_play(void);
WSADATA wsaData;

int clients = 0;
int bots = 0;
int norestart = 0;
int onceonly = 0;
int timetoplay = 0;
int is_daemon = 0;
int verbose = 0;
int level = 5;
int mode = -1;
int paused = 0;
int nospeedup = 0;

#define BUFSIZE 4096

struct user {
  unsigned int fd;
  unsigned char nick[16];
  unsigned char number;
  struct user *next, *nextvictim;
  char active;
  char introduced;
  unsigned char readbuf[BUFSIZE];
  int nread;
  unsigned char writbuf[BUFSIZE];
  int nwrite;
  char playing;
  char isbot;
  int lines;
  unsigned int games;
};

struct user *user0 = NULL;

#define NEXT(u) ((u)->next ? (u)->next : user0)

int port = DEFAULTPORT;
struct sockaddr_in saddr;
int lfd;
unsigned char realbuf[512], *buf = realbuf + 4;

int interrupt;
int tcp = -1;

fd_set fds;

/* like memcpy, but guaranteed to handle overlap when s <= t */
void copydown(char *s, char *t, int n) {
  for (; n; n--)
    *(s++) = *(t++);
}

void fatal(char *s) {
  if (!is_daemon)
    fprintf(stderr, "%s.\n", s);
  WSACleanup();
  exit(1);
}

void syserr(char *s) {
  if (!is_daemon)
    fprintf(stderr, "fatal: %s failed.\n", s);
  WSACleanup();
  exit(1);
}

void quit(void) {
	WSACleanup();
	exit(0);
}


void addtobuffer(struct user *u, unsigned char *b, int len) {
  if (u->nwrite + len >= BUFSIZE)
    fatal("Internal error: send buffer overflow");
  memcpy(u->writbuf + u->nwrite, b, len);
  u->nwrite += len;
}

void flushuser(struct user *u) {
  if (u->nwrite) {
    //write(u->fd, u->writbuf, u->nwrite);
    send(u->fd, u->writbuf, u->nwrite, 0);
    u->nwrite = 0;
  }
}

void broadcast(struct user *except, int len, int activeonly) {
  struct user *u;

  realbuf[0] = realbuf[1] = realbuf[2] = 0;
  realbuf[3] = (unsigned char)len;
  for (u=user0; u; u=u->next)
    if (u != except && (u->active || !activeonly) && u->introduced)
      addtobuffer(u, realbuf, 4 + len);
}

void sendtoone(struct user *to, int len) {
  realbuf[0] = realbuf[1] = realbuf[2] = 0;
  realbuf[3] = (unsigned char)len;
  addtobuffer(to, realbuf, 4 + len);
}

void dropuser(struct user *u) {
  struct user *v, *w;

  if (verbose)
    printf("dropping client %d (%s)\n", u->number, u->nick);
  if (u == user0)
    user0 = u->next;
  else {
    for (v=user0; v; v=v->next)
      if (v->next && v->next == u) {
	v->next = u->next;
	break;
      }
  }
  closesocket(u->fd);

  if (u->introduced) {
    buf[0] = u->number;
    buf[1] = OP_GONE;
    broadcast(u, 2, 0);
  }

  for (v=user0; v; v=v->next) {
    if (v->nextvictim == u) {
      for (w=NEXT(v); w!=v; w=NEXT(w)) {
	if (w->active && w->playing) {
	  v->nextvictim = w;
	  break;
	}
      }
      if (v->nextvictim == u)
	v->nextvictim = NULL;
    }
  }

  if (u->isbot)
    bots--;

  free(u);
  clients--;

  if (onceonly && clients == bots) {
    if (verbose)
      printf("no human clients left: exiting\n");
    quit();
  }

  if (clients == 0) {
    mode = -1;
    level = 5;
    timetoplay = 0;
  }
}

void do_play() {
  struct user *v, *w;

  for (w=user0; w; w=w->next) {
    if (w->introduced) {
      w->active = 1;
      w->playing = 1;
      w->lines = 0;
      w->nextvictim = NULL;
      for (v=NEXT(w); v!=w; v=NEXT(v)) {
	if (v->introduced) {
	  w->nextvictim = v;
	  break;
	}
      }
    }
  }
  if (paused) {
    paused = 0;
    buf[1] = OP_CONT;
    broadcast(NULL, 2, 0);
  }
  buf[1] = OP_PLAY;
  broadcast(NULL, 2, 0);
}

void new_connect(int fd) {
  struct user *u, *v;
  unsigned char nxn;

  u = malloc(sizeof (struct user));
  if (!u)
    fatal("Out of memory");
  u->fd = fd;
  u->nick[0] = 0;
  u->next = user0;
  u->nextvictim = NULL;
  u->active = 0;
  u->nread = 0;
  u->nwrite = 0;
  u->playing = 0;
  u->isbot = 0;
  u->introduced = 0;
  u->games = 0;
  user0 = u;

  nxn = 1;
again:
  v = u->next;
  while(v) {
    if (v->number == nxn) {
      nxn++;
      goto again;
    }
    v = v->next;
  }
  u->number = nxn;
  if (verbose)
    printf("client %d connecting from %s\n", nxn, inet_ntoa(saddr.sin_addr));
  clients++;
  buf[1] = OP_YOUARE;
  buf[0] = u->number;
  sendtoone(u, 2);
}

int main(int argc, char *argv[]) {
  int i, sl, on;
  struct user *u, *v, *w;
  unsigned int mfd;
  int r;
  int len;
  char *opt;
  struct protoent *tcpproto;
  struct timeval tv;

/*#ifndef NeXT
  struct sigaction sact;
#endif*/

  if ((tcpproto = getprotobyname("tcp")) != NULL)
    tcp = tcpproto->p_proto;

/*#ifdef NeXT
  signal(SIGPIPE, SIG_IGN);
#else
  sact.sa_handler = SIG_IGN;
  sigemptyset(&sact.sa_mask);
  sact.sa_flags = 0;
  sigaction(SIGPIPE, &sact, NULL);
#endif*/

  while (argc >= 2) {
    opt = argv[1];
    if (opt[0] == '-' && opt[1] == '-')
      opt++;
    if (strcmp(opt, "-h") == 0 || strcmp(opt, "-help") == 0) {
      printf(

"Use: xtserv [options]\n"
"Options:   -once       --  exit when all clients disconnect\n"
"           -v          --  be verbose\n"
"           -daemon     --  run as a daemon\n"
"           -p port     --  set server on given port\n"
"           -norestart  --  don't auto-start games\n"
"           -nospeedup  --  don't increase the speed automatically\n"

);
      exit(0);
    } else if (strcmp(opt, "-once") == 0) {
      onceonly = 1;
      argv++;
      argc--;
    } else if (strcmp(opt, "-norestart") == 0) {
      argv++;
      argc--;
      norestart = 1;
    } else if (strcmp(opt, "-nospeedup") == 0) {
      argv++;
      argc--;
      nospeedup = 1;
    } else if (strcmp(opt, "-daemon") == 0) {
      argv++;
      argc--;
      is_daemon = 1;
    } else if (strcmp(opt, "-p") == 0) {
      if (argc < 3)
	fatal("Missing argument for -p");
      port = atoi(argv[2]);
      argv += 2;
      argc -= 2;
    } else if (strcmp(opt, "-v") == 0) {
      verbose = 1;
      argv++;
      argc--;
    } else fatal("Unrecognized option, try -h for help");
  }

  //WSADATA wsaData;
  if(WSAStartup(MAKEWORD(2,0),&wsaData) != 0) {
    fatal("Socket Initialization Error. Program aborted");
  }

  lfd = socket(PF_INET, SOCK_STREAM, 0);
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_port = htons(port);

  if (lfd < 0)
    syserr("socket");
  on = 1;

  setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int));
  if (bind(lfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    syserr("bind");

  listen(lfd, 5);

  if (is_daemon) {
  	int tmp;
    char *options[5];
	char portStr[16];
	  
    for(i=0; i<5; i++) {
      options[i] = NULL;
    }
    i=0;

    if(onceonly) {
    	options[i++] = "-once";
    }
    if(port != DEFAULTPORT) {
    	options[i++] = "-p";
    	sprintf(portStr, "%d", port);
    	options[i++] = portStr;
    }
    if(norestart) {
    	options[i++] = "-norestart";
    }
    if(nospeedup) {
    	options[i++] = "-nospeedup";
    }
    	
    	
    /* become a daemon, breaking all ties with the controlling terminal */
    verbose = 0;
//    for (i=0; i<255; i++) {
//      if (i != lfd)
//		closesocket(i);
//    }


/*    if (fork())
      exit(0);
    setsid();
    if (fork())
      exit(0);
*/

		tmp = spawnlp(P_DETACH,
		#ifdef XTRISPATH
			XTRISPATH "/xtserv.exe",
		#else
			"xtserv.exe",
		#endif
			"xtserv", options[0], options[1], options[2], options[3], NULL);
		
		if(tmp == -1) {
			fprintf(stderr, "Can't start server '%s'.\n",
		#ifdef XTRISPATH
			XTRISPATH "/xtserv.exe");
		#else
			"xtserv.exe");
		#endif
			syserr("Can't start daemon server.");
		}
		else {
//    spawnlp(P_DETACH, "xtserv", "xtserv", NULL );
			for (i=0; i<255; i++) {
			  if (i != lfd)
				closesocket(i);
			}
			quit();
		}

//    spawnlp(P_NOWAIT, "xtserv", "xtserv", NULL );

    //chdir("/");
    /* open a fake stdin, stdout, stderr, just in case */
    //open("/dev/null", O_RDONLY);
    //open("/dev/null", O_WRONLY);
    //open("/dev/null", O_WRONLY);
  }

  if (verbose)
    printf("xtris server started up, listening on port %d\nusing xtris protocol version %d.%d.%d\n\n", port, PROT_VERS_1, PROT_VERS_2, PROT_VERS_3);

  while(1) {
    interrupt = 0;

    if (timetoplay && time(NULL) >= timetoplay) {
      buf[0] = 0;
      do_play();
      if (verbose)
		printf("everyone lost... restarting game\n");
      timetoplay = 0;
    }

    for (u=user0; u; u=u->next)
      flushuser(u);

    FD_ZERO(&fds);
    mfd = lfd;
    u = user0;
    while (u) {
      FD_SET(u->fd, &fds);
      if (u->fd > mfd)
	mfd = u->fd;
      u = u->next;
    }
    FD_SET((unsigned int)lfd, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    if ((sl = select(mfd + 1, &fds, NULL, NULL, &tv)) < 0)
      if (errno != EINTR)
	syserr("select");
      else continue;

    if (sl < 0)
      continue;

    if (clients > 0 && clients == bots) {
      if (verbose)
	printf("only bots left... dropping all bots\n");
      while (user0)
	dropuser(user0);
      continue;
    }

    if (sl == 0)
      continue;

    if (FD_ISSET(lfd, &fds)) {
      int newfd, slen;

      slen = sizeof(saddr);
      newfd = accept(lfd, (struct sockaddr *)&saddr, &slen);
      if (newfd < 0) {
	if (errno != EINTR)
	  syserr("accept");
      } else {
	if (tcp != -1) {
	  on = 1;
	  setsockopt(newfd, tcp, TCP_NODELAY, (char *)&on, sizeof(int));
	}
	new_connect(newfd);
      }
      continue;
    }

    u = user0;
    do {
      if (FD_ISSET(u->fd, &fds)) {
	//r = read(u->fd, u->readbuf + u->nread, BUFSIZE - u->nread);
	r = recv(u->fd, u->readbuf + u->nread, BUFSIZE - u->nread, 0);
	if (r <= 0) {
	  if (verbose)
	    printf("EOF from client %d (%s)\n", u->number, u->nick);
	  dropuser(u);
	  interrupt = 1;
	  break;
	}
	u->nread += r;
	while (u->nread >= 4 && u->nread >= 4 + u->readbuf[3]) {
	  len = u->readbuf[3];
	  if (u->readbuf[0] || u->readbuf[1] || u->readbuf[2]) {
	    if (verbose)
	      printf("crap from client %d (%s)\n", u->number, u->nick);
	    //write(u->fd, "\033]50;kanji24\007\033#8\033(0", 19);
	    send(u->fd, "\033]50;kanji24\007\033#8\033(0", 19, 0);
	    dropuser(u);
	    interrupt = 1;
	    break;
	  }
	  memcpy(buf, &u->readbuf[4], len);
	  u->nread -= 4 + len;
	  copydown(u->readbuf, u->readbuf + 4 + len, u->nread);

	  buf[0] = u->number;
	  if (!u->introduced && buf[1] != OP_NICK) {
	  	printf("Dropping user, new user didn't send nick first...\n");
	  	//printf("u->introduced: %d, buf[1]: %d\n", u->introduced, buf[1]);
	    dropuser(u);
	    interrupt = 1;
	    break;
	  }

	  switch(buf[1]) {
	    case OP_NICK:
	      if (len>16)
		len=16;
	      memcpy(u->nick, &buf[2], len-2);
	      u->nick[len-2] = 0;
	      for (i=0; i<len-2; i++) {
		if (u->nick[i] < ' ' ||
		    (u->nick[i] > 0x7e && u->nick[i] <= 0xa0)) {
		  u->nick[i] = 0;
		  break;
		}
	      }

	      if (!u->introduced) {
		buf[0] = u->number;
		buf[1] = OP_NEW;
		broadcast(u, 2, 0);
	      }

	      if (verbose)
		printf("client %d calls itself \"%s\"\n", u->number, u->nick);
	      buf[1] = OP_NICK;
	      broadcast(u, len, 0);

	      if (!u->introduced) {
		for (v=user0; v; v=v->next) {
		  if (v != u && v->introduced) {
		    buf[0] = v->number;
		    buf[1] = OP_NEW;
		    buf[2] = (v->games >> 8);
		    buf[3] = (v->games & 0xff);
		    sendtoone(u, 4);
		    buf[1] = OP_NICK;
		    memcpy(&buf[2], v->nick, 14);
		    sendtoone(u, 2+strlen(v->nick));
		  }
		}
		if (level != 5) {
		  buf[0] = 0;
		  buf[1] = OP_LEVEL;
		  buf[2] = level;
		  sendtoone(u, 3);
		}
		if (mode >= 0) {
		  buf[1] = OP_MODE;
		  buf[2] = mode;
		  sendtoone(u, 3);
		}
	      }

	      u->introduced = 1;
	      break;

	    case OP_KILL:
	      for (v=user0; v; v=v->next) {
		if (v->number == buf[2])
		  break;
	      }
	      if (v) {
		if (v->isbot) {
		  if (verbose)
		    printf("client %d (%s) kills bot %d (%s)\n", u->number, u->nick, v->number, v->nick);
		  dropuser(v);
		  interrupt = 1;
		  break;
		} else {
		  if (verbose)
		    printf("client %d (%s) attempting to kill non-bot %d (%s)\n", u->number, u->nick, v->number, v->nick);
		}
	      }
	      break;

	    case OP_PLAY:
	      if (verbose)
		printf("client %d (%s) starts game\n", u->number, u->nick);
	      timetoplay = 0;
	      do_play();
	      break;

	    case OP_MODE:
	      mode = buf[2];
	      if (verbose)
		printf("client %d (%s) sets mode %d (%s)\n", u->number, u->nick, buf[2], buf[2] == 0 ? "normal" : (buf[2] == 1 ? "fun" : "unknown"));
	      broadcast(NULL, 3, 0);
	      break;

	    case OP_PAUSE:
	      if (verbose)
		printf("client %d (%s) pauses game\n", u->number, u->nick);
	      broadcast(NULL, 2, 0);
	      paused = 1;
	      break;

	    case OP_CONT:
	      if (verbose)
		printf("client %d (%s) continues game\n", u->number, u->nick);
	      broadcast(NULL, 2, 0);
	      paused = 0;
	      break;

	    case OP_BOT:
	      if (!u->isbot)
		bots++;
	      u->isbot = 1;
	      if (verbose)
		printf("client %d (%s) declares itself to be a bot\n", u->number, u->nick);
	      break;

	    case OP_LEVEL:
	      level = buf[2];
	      if (verbose)
		printf("client %d (%s) sets level %d\n", u->number, u->nick, buf[2]);
	      broadcast(NULL, 3, 0);
	      break;

	    case OP_LOST:
	      {
		struct user *won = NULL;

		if (verbose)
		  printf("client %d (%s) has lost\n", u->number, u->nick);
		u->playing = 0;
		broadcast(u, 2, 1);
		i = 0;
		for (v=user0; v; v=v->next) {
		  if (v->nextvictim == u) {
		    for (w=NEXT(v); w!=v; w=NEXT(w)) {
		      if (w->active && w->playing) {
			v->nextvictim = w;
			break;
		      }
		    }
		    if (v->nextvictim == u)
		      v->nextvictim = NULL;
		  }
		}
		for (v=user0; v; v=v->next) {
		  if (v->playing) {
		    i++;
		    won = v;
		  }
		}
		if (i == 1) {
		  buf[0] = won->number;
		  buf[1] = OP_WON;
		  won->games++;
		  broadcast(NULL, 2, 0);
		} else if (i == 0) {
		  buf[0] = u->number;
		  buf[1] = OP_WON;
		  u->games++;
		  broadcast(NULL, 2, 0);
		}
		if (i < 2 && clients > 1 && !norestart)
		  timetoplay = time(NULL) + RESTARTDELAY;
	      }
	      break;

	    case OP_ZERO:
	      broadcast(NULL, 2, 0);
	      if (verbose)
		printf("client %d (%s) resets the game counters\n", u->number, u->nick);
	      for (v=user0; v; v=v->next)
		v->games = 0;
	      break;

	    case OP_CLEAR:
	    case OP_GROW:
	      broadcast(u, 2, 1);
	      break;

	    case OP_MSG:
	      buf[len] = 0;
	      if (verbose)
		printf("client %d (%s) sends message: %s\n", u->number, u->nick, &buf[2]);
	      broadcast(u, len, 0);
	      break;

	    case OP_DRAW:
	      broadcast(u, len, 1);
	      break;

	    case OP_FALL:
	      broadcast(u, len, 1);
	      u->lines += len - 2;
	      if (!nospeedup && u->lines > level * 10 && level < 9) {
		level++;
		buf[0] = 0;
		buf[1] = OP_LEVEL;
		buf[2] = level;
		if (verbose)
		  printf("increasing the speed to level %d\n", level);
		broadcast(NULL, 3, 0);
	      }
	      break;

	    case OP_VERSION:
	      if (len != 5 || buf[2] != PROT_VERS_1 || buf[3] != PROT_VERS_2) {
		if (verbose)
		  printf("client %d (%s) has wrong protocol version %d.%d.%d\n", u->number, u->nick, buf[2], buf[3], buf[4]);
		buf[0] = 0;
		buf[1] = OP_BADVERS;
		buf[2] = PROT_VERS_1;
		buf[3] = PROT_VERS_2;
		buf[4] = PROT_VERS_3;
		sendtoone(u, 5);
		flushuser(u);
		dropuser(u);
		interrupt = 1;
	      } else {
		if (verbose)
		  printf("client %d (%s) uses protocol version %d.%d.%d\n", u->number, u->nick, buf[2], buf[3], buf[4]);
	      }
	      break;

	    case OP_LINES:
	      if (len != 3) {
		if (verbose)
		  printf("client %d (%s) sends crap for an OP_LINES\n", u->number, u->nick);
		dropuser(u);
		interrupt = 1;
		break;
	      }
	      if (u->nextvictim) {
		if (verbose)
		  printf("client %d (%s) sends %d %s to client %d (%s)\n", u->number, u->nick, (int)buf[2], buf[2] == 1 ? "line" : "lines", u->nextvictim->number, u->nextvictim->nick);
		sendtoone(u->nextvictim, 3);
		buf[3] = u->nextvictim->number;
		buf[1] = OP_LINESTO;
		broadcast(u->nextvictim, 4, 1);
		for (v=NEXT(u->nextvictim); v!=u->nextvictim; v=NEXT(v)) {
		  if (v->active && v != u && v->playing) {
		    u->nextvictim = v;
		    break;
		  }
		}
	      } else if (verbose)
		printf("client %d (%s) makes %d %s but has no victim\n", u->number, u->nick, (int)buf[2], buf[2] == 1 ? "line" : "lines");
	      break;

	    default:
	      if (verbose)
		printf("opcode %d from client %d (%s) not understood\n", buf[0], u->number, u->nick);
	  }
	}
      }
      if (u && !interrupt)
	u = u->next;
    } while (u && !interrupt);
  }
}

