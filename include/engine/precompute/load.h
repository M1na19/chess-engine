#pragma once

#include <stdio.h>

#include "engine/engine.h"

extern BitBoard knight_look_up_table[64];
extern BitBoard king_look_up_table[64];
extern BitBoard bishop_look_up_table[64];
extern BitBoard rook_look_up_table[64];

void load_knight_bb();

void load_king_bb();

void load_bishop_bb();

void load_rook_bb();
