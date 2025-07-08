#pragma once
#include "engine.h"

void print_position(ChessPosition cp);

static inline uint8_t from_position(const char *pos);

char *move_to_str(ChessPosition cp, Move m, char out[6]);