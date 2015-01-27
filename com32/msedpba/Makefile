## -----------------------------------------------------------------------
##
##   Copyright 2001-2009 H. Peter Anvin - All Rights Reserved
##   Copyright 2009-2010 Intel Corporation; author: H. Peter Anvin
##   Copyright 2010 Shao Miller
##   Copyright 2010-2012 Michal Soltys
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as published by
##   the Free Software Foundation, Inc., 53 Temple Place Ste 330,
##   Boston MA 02111-1307, USA; either version 2 of the License, or
##   (at your option) any later version; incorporated herein by reference.
##
## -----------------------------------------------------------------------

VPATH = $(SRC)
include $(MAKEDIR)/elf.mk

OBJS = msedpba.o ahci.o sata.o Msed.o unlockOpal.o trace.o
GNUOBJS = sha1.o hmac-sha1.o memxor.o gc-gnulib.o gc-pbkdf2-sha1.o
CHAINOBJS = chain.o partiter.o utility.o options.o mangle.o
MENUOBJS = getpass.o

OBJS += $(GNUOBJS)
OBJS += $(CHAINOBJS)
OBJS += $(MENUOBJS)

CFLAGS += -fno-strict-aliasing -Werror -Wall

all: msedpba.c32

msedpba.elf: $(OBJS) $(C_LIBS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(MAKEDEPS) $(CFLAGS) $(CHAINEXTOPT) -c -o $@ $<

tidy dist:
	rm -f *.o *.lo *.a *.lst *.elf .*.d *.tmp

clean: tidy
	rm -f *.lnx

spotless: clean
	rm -f *.lss *.c32 *.com
	rm -f *~ \#*

install:


-include .*.d
