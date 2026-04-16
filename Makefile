# Makefile for SLOsh - San Luis Obispo Shell
# CSC 453 - Operating Systems

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=700
# Source files
SRCS = slosh.c
# Executable name
EXEC = slosh

# Default target to build the shell
all: $(EXEC)
$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Clean target to remove the executable
clean:
	rm -f $(EXEC)
	rm -f slosh_tests

fresh:
	make clean
	make all
	./slosh