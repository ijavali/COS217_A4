#--------------------------------------------------------------------
# Makefile for Assignment 4, Part 3
# Author: Ishaan Javali
#--------------------------------------------------------------------

CC=gcc217

all: sampleft ft

clean:
	rm -f sampleft ft

clobber: clean
	rm -f ft_client.o ft.o *~

sampleft: sampleft.o ft_client.o
	$(CC) sampleft.o ft_client.o -o sampleft

ft_client.o: ft_client.c ft.h nodeFT.h a4def.h
	$(CC) -c ft_client.c

ft: ft.o ft_client.o
	$(CC) ft.o ft_client.o -o ft

ft.o: ft.c nodeFT.h
	$(CC) -c ft.c
