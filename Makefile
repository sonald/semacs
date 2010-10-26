LDFLAGS= `pkg-config x11 glib-2.0 xft --libs`
CFLAGS= `pkg-config x11 glib-2.0 xft --cflags` -g -Wall -std=c99

all: semacs

semacs: main.o 
	$(CC) -o $@ $^ $(LDFLAGS) 

