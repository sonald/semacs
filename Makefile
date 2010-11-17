CC=gcc
CXX=g++
LDFLAGS=`pkg-config x11 glib-2.0 xft QtGui --libs`
CFLAGS=`pkg-config x11 glib-2.0 xft QtGui --cflags` -g -Wall -std=gnu99
#CFLAGS=`pkg-config x11 glib-2.0 xft QtGui --cflags` -g -Wall -std=c++98
CXXFLAGS=`pkg-config x11 glib-2.0 xft QtGui --cflags` -g -Wall -std=c++98

MOC=moc

HFILES= util.h \
	editor.h \
	env.h \
	modemap.h \
	buffer.h \
	key.h \
	cmd.h \
	xview.h \
	qview.h \
	view.h \
	submatch.h

OFILES= \
	obj/xview.o \
	obj/env.o \
	obj/editor.o \
	obj/buffer.o \
	obj/modemap.o \
	obj/key.o \
	obj/cmd.o \
	obj/multimatch.o \
	obj/view.o \
	obj/qview.o \
	obj/singlematch.o \
	obj/qview.moc.o


TESTOFILES= \
	obj/testsuites.o


all: semacs test_semacs

test_semacs: $(TESTOFILES) $(OFILES)
	$(CXX) -o $@ $^ $(LDFLAGS)

semacs: $(OFILES) obj/main.o
	$(CXX) -o $@ $^ $(LDFLAGS) 


obj/%.o: %.c $(HFILES)
	@mkdir -p $$(dirname $@)
	$(CC) -c -o $@ $(CFLAGS) $*.c

obj/%.o: %.cc $(HFILES)
	@mkdir -p $$(dirname $@)
	$(CXX) -c -o $@ $(CXXFLAGS) $*.cc

qview.o: qview.moc.cc

qview.moc.cc: qview.h
	$(MOC) $^ -o $@

clean:
	rm -rf obj
	-rm semacs
	-rm test_semacs


