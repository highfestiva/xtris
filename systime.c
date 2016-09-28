/*
 *  Made as replacement for the standard Unix <sys/time.h> file used for
 *  porting xtris to win32.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation. See the file COPYING for details.
 *
 */

//#include "systime.h"
#include <winsock2.h>
#include <time.h>

int gettimeofday(struct timeval *__p, struct timezone *__z) {
	int result = 0;
	SYSTEMTIME systemtime;
	GetSystemTime(&systemtime);
	__p->tv_usec = systemtime.wMilliseconds * 1000;
	__p->tv_sec = time(NULL);
	return result;
}

