### GameCam v1.04 Linux Makefile ###

# Cascade Proxy Technique:
# This mod will name itself gamei386.so or gamex86_64.so under
# Linux according to the build mode you choose. It will attempt
# to load q2admin.so in the mod folder chosen by the dedicated
# server startup command line. Q2admin will then attempt to load
# the actual game mod, expecting it to be named gamei386.real.so 
# or gamex86_64.real.so.

# Load order: 
# server
# gamei386.so (this module)
# q2admin.so (the q2admin mod)
# gamei386.real.so (your game mod)

# this nice line comes from the linux kernel makefile
ARCH := $(shell uname -m | sed -e s/i.86/i386/ \
	-e s/sun4u/sparc64/ -e s/arm.*/arm/ \
	-e s/sa110/arm/ -e s/alpha/axp/)

# On 64-bit OS use the command: setarch i386 make all
# to obtain the 32-bit binary DLL on 64-bit Linux.

CC = gcc -std=c17 -Wall

# on x64 machines do this preparation:
# sudo apt-get install ia32-libs
# sudo apt-get install libc6-dev-i386
# On Ubuntu 16.x use sudo apt install libc6-dev-i386
# this will let you build 32-bits on ia64 systems
#
# This is for native build
CFLAGS=-O3 -DARCH="$(ARCH)" -DSTDC_HEADERS
# This is for 32-bit build on 64-bit host
ifeq ($(ARCH),i386)
CFLAGS =-m32 -O3 -DARCH="$(ARCH)" -DSTDC_HEADERS -I/usr/include
endif

# use this when debugging
#CFLAGS=-g -Og -DDEBUG -DARCH="$(ARCH)" -Wall -pedantic

# This selects whether we want to use q2admin with our server.
CFLAGS += -DQ2ADMIN

# flavors of Linux
ifeq ($(shell uname),Linux)
#SVNDEV := -D'SVN_REV="$(shell svnversion -n .)"'
#CFLAGS += $(SVNDEV)
CFLAGS += -DLINUX
LIBTOOL = ldd
endif

# OS X wants to be Linux and FreeBSD too.
ifeq ($(shell uname),Darwin)
#SVNDEV := -D'SVN_REV="$(shell svnversion -n .)"'
#CFLAGS += $(SVNDEV)
CFLAGS += -DLINUX
LIBTOOL = otool
endif

LDFLAGS=-ldl -shared -o
SHLIBEXT=so
#set position independent code
SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared

DO_SHLIB_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<

.c.o:
	$(DO_SHLIB_CC)

GAME_OBJS_GC = \
	gc_action.o gc_chase.o gc_cmd.o gc_config.o gc_connect.o \
	gc_creep.o gc_fixed.o gc_frame.o gc_free.o gc_id.o \
	gc_main.o gc_menu.o gc_net.o gc_ticker.o gc_utils.o

game$(ARCH).$(SHLIBEXT) : $(GAME_OBJS_GC)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(GAME_OBJS_GC) -ldl -lm
	ldd -r $@


#############################################################################
# MISC
#############################################################################

clean:
	-rm -f $(GAME_OBJS_GC)

all:
	-rm -f $(GAME_OBJS_GC)
	make

depends:
	$(CC) $(CFLAGS) -MM *.c > dependencies

#############################################################################
# DEPENDENCIES
#############################################################################

### GameCam

-include dependencies
