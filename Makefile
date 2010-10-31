CC=gcc
LDFLAGS=`pkg-config x11 glib-2.0 xft --libs`
CFLAGS=`pkg-config x11 glib-2.0 xft --cflags` -g -Wall -std=gnu99

HFILES=util.h \
	editor.h \
	xview.h

# OFILES= obj/editor.o \
# 	obj/xview.o \
# 	obj/main.o

OFILES=xview.o \
	util.o \
	env.o \
	editor.o \
	buffer.o \
	main.o


all: semacs

semacs: $(OFILES)
	$(CC) -o $@ $^ $(LDFLAGS) 


obj/%.o: %.c $(HFILES)
	$(CC) -o $@ $(CFLAGS) $*.c


clean:
	rm -rf obj
	rm *.o
	-rm semacs


