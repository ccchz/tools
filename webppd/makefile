CC = gcc

ifeq ($(MSYSTEM),MINGW32)
	LIBDIR = -L../x86
else
	LIBDIR = -L../amd64
	CFLAGS += -D_WIN32	\
		  -D__MINGW32__	\
		  -m64
endif

CFLAGS += -Wall \
          -Wextra \
	  -O3	\
          -Wno-unused-parameter \
          -Iinclude \
          -Ihttp_parser

SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "debug") print $$9}')

DEBUG=debug 
#$(shell ls -l | grep ^d | awk '{if($$9 == "debug") print $$9}')

ROOT_DIR=$(shell pwd)

BIN = webppd.exe

OBJS_DIR = debug/obj

BIN_DIR = debug/bin

CUR_SOURCE=${wildcard *.c}

CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}

export CC BIN OBJS_DIR BIN_DIR ROOT_DIR CFLAGS LIBDIR

all:$(SUBDIRS) $(CUR_OBJS) $(DEBUG)

$(SUBDIRS):ECHO
	make -C $@
$(DEBUG):ECHO
	make -C $@
ECHO:
	@echo  $(SUBDIRS)

$(CUR_OBJS):%.o:%.c
	$(CC) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@
clean:
	@rm  $(OBJS_DIR)/*.o
	@rm  -rf $(BIN_DIR)/*
