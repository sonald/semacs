CC=gcc
LDFLAGS=`pkg-config x11 glib-2.0 xft --libs`
CFLAGS=`pkg-config x11 glib-2.0 xft --cflags` -g -Wall -std=gnu99

HFILES=util.h \
	editor.h \
	xview.h

OFILES= obj/xview.o \
	obj/util.o \
	obj/env.o \
	obj/editor.o \
	obj/buffer.o \
	obj/main.o


all: semacs

semacs: $(OFILES)
	$(CC) -o $@ $^ $(LDFLAGS) 


obj/%.o: %.c $(HFILES)
	@mkdir -p $$(dirname $@)
	$(CC) -c -o $@ $(CFLAGS) $*.c


clean:
	rm -rf obj
	rm *.o
	-rm semacs


