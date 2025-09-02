# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -Wextra -std=c99 -O3 -march=native -fno-exceptions -DNDEBUG -Iinclude

.PHONY: all clean directories precompute

all: chess precompute test

directories:
	mkdir -p build data

# Main process
SRCS := $(shell find src/debug src/moves -name '*.c') src/main.c src/precompute/load.c src/engine.c

OBJS := $(SRCS:.c=.o)

TARGET = chess

$(TARGET): directories build/$(TARGET)

build/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm



# Precompute
SRCS_PRECOMPUTE := $(shell find src/precompute -name '*.c')

OBJS_PRECOMPUTE := $(SRCS_PRECOMPUTE:.c=.o)

TARGET_PRECOMPUTE=precompute

$(TARGET_PRECOMPUTE): directories build/$(TARGET_PRECOMPUTE)

build/$(TARGET_PRECOMPUTE): $(OBJS_PRECOMPUTE)
	$(CC) $(CFLAGS) $^ -o $@ -lm

# Compile each source file into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(OBJS) $(OBJS_PRECOMPUTE)