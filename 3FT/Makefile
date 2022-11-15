#--------------------------------------------------------------------
# Makefile for Assignment 4, Part 3
# Author: Ishaan Javali
#--------------------------------------------------------------------

CC=gcc217

all: sampleft ft

clean:
	rm -f sampleft ft

clobber: clean
	rm -f path.o dynarray.o nodeFT.o ft.o ft_client.o *~

sampleft: sampleft.o ft_client.o
	$(CC) sampleft.o ft_client.o -o sampleft

ft: ft.o ft_client.o path.o dynarray.o nodeFT.o
	$(CC) ft.o ft_client.o path.o dynarray.o nodeFT.o -o ft

ft_client.o: ft_client.c ft.h nodeFT.h a4def.h
	$(CC) -g -c ft_client.c

ft.o: ft.c nodeFT.h dynarray.h path.h ft.h a4def.h
	$(CC) -g -c ft.c

nodeFT.o: nodeFT.c dynarray.h path.h nodeFT.h path.h a4def.h
	$(CC) -g -c nodeFT.c

dynarray.o: dynarray.c
	$(CC) -g -c dynarray.c

path.o: path.c path.h dynarray.h
	$(CC) -g -c path.c

#	gcc217 ft.c path.c dynarray.c nodeFT.c ft_client.c -o ft