/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2004-2008 H. Peter Anvin - All Rights Reserved
 *   Copyright 2009-2014 Intel Corporation; author: H. Peter Anvin
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *   Boston MA 02110-1301, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

/*
 * stolen from menumain.c
 * and then added errors ;-}
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <consoles.h>
#include <getkey.h>
#include <minmax.h>
#include <setjmp.h>
#include <limits.h>
#include <com32.h>
#include <core.h>
#include <syslinux/adv.h>
#include <syslinux/boot.h>
#include <sys/cpu.h>

static jmp_buf timeout_jump;


int mygetkey(clock_t timeout)
{
    long long totaltimeout = 0;
    clock_t t0, t;
    clock_t tto, to;
    int key;

    if (!totaltimeout)
	return get_key(stdin, timeout);

    for (;;) {
	tto = min(totaltimeout, INT_MAX);
	to = timeout ? min(tto, timeout) : tto;

	t0 = times(NULL);
	key = get_key(stdin, to);
	t = times(NULL) - t0;

	if (totaltimeout <= t)
	    longjmp(timeout_jump, 1);

	totaltimeout -= t;

	if (key != KEY_NONE)
	    return key;

	if (timeout) {
	    if (timeout <= t)
		return KEY_NONE;

	    timeout -= t;
	}
    }
}
void drain_keyboard(void)
{
    /* Prevent "ghost typing" and keyboard buffer snooping */
    volatile char junk;
    int rv;

    do {
	rv = read(0, (char *)&junk, 1);
    } while (rv > 0);

    junk = 0;

    cli();
    *(volatile uint8_t *)0x419 = 0;	/* Alt-XXX keyboard area */
    *(volatile uint16_t *)0x41a = 0x1e;	/* Keyboard buffer empty */
    *(volatile uint16_t *)0x41c = 0x1e;
    memset((void *)0x41e, 0, 32);	/* Clear the actual keyboard buffer */
    sti();
}
#define WIDTH 256
int ask_passwd(char *user_passwd)
{
    char *p;
    int done;
    int key;
    drain_keyboard();
    memset(user_passwd, 0, WIDTH);
    done = 0;
    p = user_passwd;

    while (!done) {
	key = mygetkey(0);

	switch (key) {
	case KEY_ENTER:
	case KEY_CTRL('J'):
	    done = 1;
	    break;

	case KEY_ESC:
	case KEY_CTRL('C'):
	    p = user_passwd;	/* No password entered */
	    done = 1;
	    break;

	case KEY_BACKSPACE:
	case KEY_DEL:
	case KEY_DELETE:
	    if (p > user_passwd) {
		printf("\b \b");
		p--;
	    }
	    break;

	case KEY_CTRL('U'):
	    while (p > user_passwd) {
		printf("\b \b");
		p--;
	    }
	    break;

	default:
	    if (key >= ' ' && key <= 0xFF &&
		(p - user_passwd) < WIDTH - 1) {
		*p++ = key;
		putchar('*');
	    }
	    break;
	}
    }

    if (p == user_passwd)
	return 1;		/* No password entered */

    drain_keyboard();

    return 0;
}
