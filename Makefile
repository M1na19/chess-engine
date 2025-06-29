# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -Wextra -std=c99 -O0 -Iinclude

# Source files (recursive wildcard)
SRCS := $(wildcard src/*.c) \
        $(wildcard src/engine/*.c)

# Object files
OBJS := $(SRCS:.c=.o)

# Output executable
TARGET = build/chess

.PHONY: all clean directories

all: directories $(TARGET)

directories:
	mkdir -p build

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

# Compile each source file into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(OBJS)
