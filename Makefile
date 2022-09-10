CC = gcc
CFLAGS = -Wall -lm -fPIC -O2

libdigestauth: libdigestauth.o
	$(CC) -shared libdigestauth.o -o libdigestauth.so -s -lcurl

libdigestauth.o: main.c
	$(CC) $(CFLAGS) -c main.c -o libdigestauth.o

all:
	libdigestauth

clean: 
	rm -f *.o main1
