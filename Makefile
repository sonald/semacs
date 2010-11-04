CC=gcc
LDFLAGS=`pkg-config x11 glib-2.0 xft --libs`
CFLAGS=`pkg-config x11 glib-2.0 xft --cflags` -g -Wall -std=gnu99

HFILES= util.h \
	editor.h \
	env.h \
	modemap.h \
	buffer.h \
	key.h \
	cmd.h \
	xview.h \
	submatch.h

OFILES= \
	obj/xview.o \
	obj/util.o \
	obj/env.o \
	obj/editor.o \
	obj/buffer.o \
	obj/modemap.o \
	obj/key.o \
	obj/cmd.o \
	obj/multimatch.o \
	obj/singlematch.o


TESTOFILES= \
	obj/testsuites.o


all: semacs test_semacs

test_semacs: $(TESTOFILES) $(OFILES)
	$(CC) -o $@ $^ $(LDFLAGS)

semacs: $(OFILES) obj/main.o
	$(CC) -o $@ $^ $(LDFLAGS) 


obj/%.o: %.c $(HFILES)
	@mkdir -p $$(dirname $@)
	$(CC) -c -o $@ $(CFLAGS) $*.c


clean:
	rm -rf obj
	rm *.o
	-rm semacs


