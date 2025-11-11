# Makefile for File Tree (FT)
# Author: [Your Name]

# Executable name
TARGET = ft

# Compiler and flags
CC = gcc217
CFLAGS = -g

# Object files needed
OBJS = dynarray.o path.o nodeFT.o ft.o ft_client.o

# Default target: build the ft executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Individual object file rules with dependencies

dynarray.o: dynarray.c dynarray.h
	$(CC) $(CFLAGS) -c dynarray.c

path.o: path.c path.h a4def.h
	$(CC) $(CFLAGS) -c path.c

nodeFT.o: nodeFT.c nodeFT.h a4def.h path.h dynarray.h
	$(CC) $(CFLAGS) -c nodeFT.c

ft.o: ft.c ft.h nodeFT.h a4def.h path.h dynarray.h
	$(CC) $(CFLAGS) -c ft.c

ft_client.o: ft_client.c ft.h a4def.h
	$(CC) $(CFLAGS) -c ft_client.c

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)


