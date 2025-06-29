#pragma once

#include <stdio.h>

#include "engine/engine.h"

BitBoard knight_look_up_table[64];
BitBoard king_look_up_table[64];

void load_knight_bb();

void load_king_bb();
