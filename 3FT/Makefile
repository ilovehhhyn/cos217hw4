#--------------------------------------------------------------------
# Makefile for Assignment 4, Part 3
# Author: Helen Huang
#--------------------------------------------------------------------
CC=gcc217

all: ft

clean:
	rm -f dynarray.o path.o nodeFT.o ft.o ft_client.o ft

ft: dynarray.o path.o nodeFT.o ft.o ft_client.o
	$(CC) dynarray.o path.o nodeFT.o ft.o ft_client.o -o ft

dynarray.o: dynarray.c dynarray.h
	$(CC) -c dynarray.c

path.o: path.c path.h a4def.h
	$(CC) -c path.c

nodeFT.o: nodeFT.c nodeFT.h a4def.h path.h dynarray.h
	$(CC) -c nodeFT.c

ft.o: ft.c ft.h nodeFT.h a4def.h path.h dynarray.h
	$(CC) -c ft.c

ft_client.o: ft_client.c ft.h a4def.h
	$(CC) -c ft_client.c
