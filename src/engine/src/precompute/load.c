#include "precompute/load.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

BitBoard knight_look_up_table[64];
BitBoard king_look_up_table[64];
MagicVec bishop_look_up_table[64];
MagicVec rook_look_up_table[64];

BitBoard get_attack_magic_vec(MagicVec mv, BitBoard occ) {
  int idx = ((occ & mv.attack_mask) * mv.magic_num) >> (64 - mv.nr_bits);
  return VALUE(BitBoard, vector_get(&mv.val, idx));
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
    BitBoard attack_mask;
    fread(&attack_mask, sizeof(attack_mask), 1, in);
    uint64_t m_num;
    fread(&m_num, sizeof(m_num), 1, in);
    uint8_t bits;
    fread(&bits, sizeof(bits), 1, in);

    uint32_t table_size = 1 << bits;
    Vector values = vector_init(sizeof(BitBoard), table_size);
    fread(values.data, sizeof(BitBoard), table_size, in);
    values.count = table_size;

    bishop_look_up_table[i].val = values;
    bishop_look_up_table[i].magic_num = m_num;
    bishop_look_up_table[i].nr_bits = bits;
    bishop_look_up_table[i].attack_mask = attack_mask;
  }

  fclose(in);
}

void load_rook_bb() {
  FILE *in = fopen("./data/rook.bb", "rb");
  for (int i = 0; i < 64; i++) {
    BitBoard attack_mask;
    fread(&attack_mask, sizeof(attack_mask), 1, in);
    uint64_t m_num;
    fread(&m_num, sizeof(m_num), 1, in);
    uint8_t bits;
    fread(&bits, sizeof(bits), 1, in);

    uint32_t table_size = 1 << bits;
    Vector values = vector_init(sizeof(BitBoard), table_size);
    fread(values.data, sizeof(BitBoard), table_size, in);
    values.count = table_size;

    rook_look_up_table[i].val = values;
    rook_look_up_table[i].magic_num = m_num;
    rook_look_up_table[i].nr_bits = bits;
    rook_look_up_table[i].attack_mask = attack_mask;
  }

  fclose(in);
}
