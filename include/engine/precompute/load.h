#pragma once

#include <stdio.h>

#include "engine/engine.h"

extern BitBoard knight_look_up_table[64];
extern BitBoard king_look_up_table[64];
typedef struct magic_vec {
  Vector val;
  BitBoard attack_mask;
  uint64_t magic_num;
  int nr_bits;
} MagicVec;
extern MagicVec bishop_look_up_table[64];
extern MagicVec rook_look_up_table[64];

BitBoard get_attack_magic_vec(MagicVec mv, BitBoard occ);

void load_knight_bb();

void load_king_bb();

void load_bishop_bb();

void load_rook_bb();
