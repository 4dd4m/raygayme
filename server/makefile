# Compiler
CC = gcc

# Target
TARGET = server.exe

# Source files
SRC = main.c $(wildcard src/*.c) $(wildcard ../shared/src/*.c) ../client/src/cJSON.c

# Compiler flags
CFLAGS = -O1 -Wall -std=c99 -Wno-missing-braces -Iinclude -I../shared/include -I../client/include

# Linker flags
LDFLAGS = -Llib -lraylib -lopengl32 -lgdi32 -lws2_32 -lwinmm -ggdb

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

# Remove build files
clear:
	del /Q $(TARGET) 2>nul

# Alias commonly used by make
clean: clear

.PHONY: all clear clean
