# Compiler
CC = gcc

# Target
TARGET = game.exe

# Source files
SRC = main.c

# Compiler flags
CFLAGS = -O1 -Wall -std=c99 -Wno-missing-braces -Iinclude

# Linker flags
LDFLAGS = -Llib -lraylib -lopengl32 -lgdi32 -lwinmm -ggdb

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