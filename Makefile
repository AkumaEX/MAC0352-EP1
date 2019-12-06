CC		= gcc
CFLAGS	= -Wall -ansi -pedantic -O2

server: server.o functions.o
		$(CC) $(CFLAGS) -o $@ $^

clean:	
		rm -f *.o server

cleanall:
		rm -f *.o server
		@ rm -r -- */