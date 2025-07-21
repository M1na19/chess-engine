# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -Wextra -std=c99 -Ofast -march=native -Iinclude

.PHONY: all clean directories precompute

all: chess precompute test

directories:
	mkdir -p build data

# Main process
SRCS := $(shell find src/engine src/error src/vector -name '*.c') src/main.c src/precompute/load.c

OBJS := $(SRCS:.c=.o)

TARGET = chess

$(TARGET): directories build/$(TARGET)

build/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm



# Precompute
SRCS_PRECOMPUTE := $(shell find src/precompute src/vector src/error src/engine -name '*.c')

OBJS_PRECOMPUTE := $(SRCS_PRECOMPUTE:.c=.o)

TARGET_PRECOMPUTE=precompute

$(TARGET_PRECOMPUTE): directories build/$(TARGET_PRECOMPUTE)

build/$(TARGET_PRECOMPUTE): $(OBJS_PRECOMPUTE)
	$(CC) $(CFLAGS) $^ -o $@ -lm



# Tests
SRCS_TEST := $(shell find src/engine src/vector src/error src/evaluate tests -name '*.c') src/precompute/load.c

OBJS_TEST := $(SRCS_TEST:.c=.o)

TARGET_TEST=test

$(TARGET_TEST): directories build/$(TARGET_TEST)

build/$(TARGET_TEST): $(OBJS_TEST)
	$(CC) $(CFLAGS) $^ -o $@ -lm


# Compile each source file into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(OBJS) $(OBJS_PRECOMPUTE) $(OBJS_TEST) 