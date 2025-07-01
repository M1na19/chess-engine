#include "engine/precompute/load.h"
#include "error.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
BitBoard knight_look_up_table[64];
BitBoard king_look_up_table[64];
MagicVec bishop_look_up_table[64];
MagicVec rook_look_up_table[64];

BitBoard get_attack_magic_vec(MagicVec mv, BitBoard occ) {
  int idx = (occ * mv.magic_num) >> (64 - mv.nr_bits);
  return VALUE(BitBoard, get_vector(mv.val, idx));
}

void load_knight_bb() {
  FILE *in = fopen("./data/knight.bb", "r");
  fread(knight_look_up_table, sizeof(BitBoard), 64, in);
  fclose(in);
}

void load_king_bb() {
  FILE *in = fopen("./data/king.bb", "r");
  fread(king_look_up_table, sizeof(BitBoard), 64, in);
  fclose(in);
}

void load_bishop_bb() {
  FILE *in = fopen("./data/bishop.bb", "rb");
  for (int i = 0; i < 64; i++) {
    uint64_t m_num;
    fread(&m_num, sizeof(m_num), 1, in);
    uint8_t bits;
    fread(&bits, sizeof(bits), 1, in);

    uint32_t table_size = 1 << bits;
    Vector values = f_malloc(sizeof(struct vector));
    init_vector(values, sizeof(BitBoard), table_size);
    fread(values->data, sizeof(BitBoard), table_size, in);
    values->count = table_size;

    bishop_look_up_table[i].val = values;
    bishop_look_up_table[i].magic_num = m_num;
    bishop_look_up_table[i].nr_bits = bits;
  }

  fclose(in);
}

void load_rook_bb() {
  FILE *in = fopen("./data/rook.bb", "rb");
  for (int i = 0; i < 64; i++) {
    uint64_t m_num;
    fread(&m_num, sizeof(m_num), 1, in);
    uint8_t bits;
    fread(&bits, sizeof(bits), 1, in);

    uint32_t table_size = 1 << bits;
    Vector values = f_malloc(sizeof(struct vector));
    init_vector(values, sizeof(BitBoard), table_size);
    fread(values->data, sizeof(BitBoard), table_size, in);
    values->count = table_size;

    rook_look_up_table[i].val = values;
    rook_look_up_table[i].magic_num = m_num;
    rook_look_up_table[i].nr_bits = bits;
  }

  fclose(in);
}
