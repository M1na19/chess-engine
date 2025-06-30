# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -Wextra -std=c99 -O0 -Iinclude

# Source files (recursive wildcard)
SRCS := $(shell find src -name '*.c')
SRCS := $(filter-out src/engine/precompute/precompute.c, $(SRCS))

# Object files
OBJS := $(SRCS:.c=.o)

# Output executable
TARGET = build/chess
TARGET_PRECOMPUTE=build/precompute
.PHONY: all clean directories

all: directories $(TARGET)

precompute: directories $(TARGET_PRECOMPUTE)

directories:
	mkdir -p build

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(TARGET_PRECOMPUTE): src/engine/precompute/precompute.c src/vector/vector.c src/error/error.c
	$(CC) -Wall -g -Wextra -std=c99 -Ofast -Iinclude $^ -o $@ -lm

# Compile each source file into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(OBJS)
