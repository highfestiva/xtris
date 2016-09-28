/*
 *  Made as replacement for the standard Unix <sys/time.h> file used for 
 *  porting xtris to win32.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation. See the file COPYING for details.
 * 
 */


#ifndef _SYSTIME_H_
#define _SYSTIME_H_

#include <winsock2.h>
//#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


int gettimeofday(struct timeval *__p, struct timezone *__z);

#ifdef __cplusplus
}
#endif

#endif /* _SYSTIME_H_ */
