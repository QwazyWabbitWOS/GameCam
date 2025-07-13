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

.DEFAULT_GOAL := game

# this nice line comes from the linux kernel makefile
ARCH := $(shell uname -m | sed -e s/i.86/i386/ \
	-e s/sun4u/sparc64/ -e s/arm.*/arm/ \
	-e s/sa110/arm/ -e s/alpha/axp/)

# On 64-bit OS use the command: setarch i386 make all
# to obtain the 32-bit binary DLL on 64-bit Linux.

CC = gcc -std=c11 -Wall

# on x64 machines do this preparation:
# sudo apt-get install ia32-libs
# sudo apt-get install libc6-dev-i386
# On Ubuntu 16.x and higher use sudo apt install libc6-dev-i386
# this will let you build 32-bits on ia64 systems
#
# This is for native build
CFLAGS=-O3 -DARCH="$(ARCH)" -DSTDC_HEADERS
# This is for 32-bit build on 64-bit host
ifeq ($(ARCH),i386)
CFLAGS += -m32 -I/usr/include
endif

# use this when debugging
#CFLAGS=-g -Og -DDEBUG -DARCH="$(ARCH)" -Wall -pedantic

# This selects whether we want to use q2admin with our server.
CFLAGS += -DQ2ADMIN

# flavors of Linux
ifeq ($(shell uname),Linux)
CFLAGS += -DLINUX
LIBTOOL = ldd
endif

# OS X wants to be Linux and FreeBSD too.
ifeq ($(shell uname),Darwin)
CFLAGS += -DLINUX
LIBTOOL = otool
endif

LDFLAGS=-ldl -shared -o
SHLIBEXT=so
#set position independent code
SHLIBCFLAGS=-fPIC

# Build directory
BUILD_DIR = build$(ARCH)

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
DO_SHLIB_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<


# List of source and object files
GAME_SRCS = \
	gc_action.c gc_chase.c gc_cmd.c gc_config.c gc_connect.c \
	gc_creep.c gc_fixed.c gc_frame.c gc_free.c gc_id.c \
	gc_main.c gc_menu.c gc_net.c gc_ticker.c gc_utils.c

GAME_OBJS = $(GAME_SRCS:%.c=$(BUILD_DIR)/%.o)

# Pattern rule to place objects in build directory
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SHLIBCFLAGS) -MMD -MP -MF $(@:.o=.d) -o $@ -c $<
-include $(GAME_OBJS:.o=.d)
# Build all object files that are out-of-date
game: $(GAME_OBJS) game$(ARCH).$(SHLIBEXT)


# Main target: depends on all object files
game$(ARCH).$(SHLIBEXT): $(GAME_OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $(GAME_OBJS) -ldl -lm
	$(LIBTOOL) -r $@
	file $@

# Build everything (always rebuild all objects and the shared library)
all:
	$(MAKE) clean
	$(MAKE) $(BUILD_DIR)
	$(MAKE) $(GAME_OBJS)
	$(MAKE) game$(ARCH).$(SHLIBEXT)

#############################################################################
# MISC
#############################################################################

clean:
	rm -rf $(BUILD_DIR)


