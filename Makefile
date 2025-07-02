# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -Wextra -std=c99 -Ofast -march=native -Iinclude

# Source files (recursive wildcard)
SRCS := $(shell find src -name '*.c')
SRCS := $(filter-out src/engine/precompute/precompute.c, $(SRCS))

# Object files
OBJS := $(SRCS:.c=.o)

# Output executable
TARGET = build/chess
TARGET_PRECOMPUTE=build/precompute

.PHONY: all clean directories precompute

all: directories $(TARGET) $(TARGET_PRECOMPUTE)



directories:
	mkdir -p build

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

# Compile each source file into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(OBJS)


# Source files (recursive wildcard)
SRCS_PRECOMPUTE := $(shell find src -name '*.c')
SRCS_PRECOMPUTE := $(filter-out src/main.c, $(SRCS_PRECOMPUTE))

# Object files
OBJS_PRECOMPUTE := $(SRCS_PRECOMPUTE:.c=.o)

precompute: directories $(TARGET_PRECOMPUTE)

$(TARGET_PRECOMPUTE): $(OBJS_PRECOMPUTE)
	$(CC) $(CFLAGS) $^ -o $@ -lm