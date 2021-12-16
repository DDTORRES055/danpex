CC=gcc
CFLAGS=-O3 -Wall

danpex : sha256.o danpex.o
	$(CC) $(CFLAGS) -o danpex sha256.o danpex.o

sha256.o : sha256.c sha256.h
	$(CC) $(CFLAGS) -c sha256.c

danpex.o : danpex.c danpex.h
	$(CC) $(CFLAGS) -c danpex.c
clean :
	rm danpex danpex.o sha256.o