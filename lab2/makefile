CC = gcc 
INCLUDES = -I/home/jplank/cs360/include
CFLAGS = $(INCLUDES)
LIBDIR = /home/jplank/cs360/lib
LIBS = $(LIBDIR)/libfdr.a 

EXECUTABLES = l2p1 l2p2 l2p3

all: $(EXECUTABLES)

l2p1: l2p1.c
	$(CC) $(CFLAGS) -g  -ggdb3 -o l2p1 l2p1.c $(LIBS)

l2p2: l2p2.c
	$(CC) $(CFLAGS) -g  -ggdb3 -o l2p2 l2p2.c $(LIBS)

l2p3: l2p3.c
	$(CC) $(CFLAGS) -g  -ggdb3 -o l2p3 l2p3.c $(LIBS)

clean:
	rm core $(EXECUTABLES) *.o
