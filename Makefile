### GameCam v1.04 Linux Makefile ###
ARCH=i386

# on x64 machines do this preparation:
# sudo apt-get install ia32-libs
# sudo apt-get install libc6-dev-i386
# this will let you build 32-bits on ia64 systems
#

#use these cflags to optimize this build
CFLAGS=-O3 -m32 -DARCH=\"$(ARCH)\" -Wextra
#use these when debugging 
#CFLAGS=-g -m32 -DARCH=\"$(ARCH)\" -Wall

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

DO_CC=$(CC) $(CFLAGS) -o $@ -c $<
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

depends:
	$(CC) $(CFLAGS) -MM *.c > dependencies

#############################################################################
# DEPENDENCIES
#############################################################################

### GameCam

-include dependencies
