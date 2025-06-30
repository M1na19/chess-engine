#include "../../../include/engine/precompute/load.h"
#include "engine/precompute/load.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

BitBoard knight_look_up_table[64];
BitBoard king_look_up_table[64];

size_t get_bitboard_count(FILE *f) {
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    if (size < 0) {
        perror("ftell failed");
        return 0;
    }
    return size / sizeof(BitBoard);
}

void load_knight_bb() {
    FILE *in = fopen("data/knight.bb", "r");
    fread(knight_look_up_table, sizeof(BitBoard), 64, in);
    fclose(in);
}

void load_king_bb() {
    FILE *in = fopen("data/king.bb", "r");
    fread(king_look_up_table, sizeof(BitBoard), 64, in);
    fclose(in);
}

void load_bishop_bb() {
    FILE *in = fopen("data/bishop.bb", "r");
    fread(bishop_look_up_table, sizeof(BitBoard), 64, in);
    fclose(in);
}

void load_rook_bb() {
    FILE *in = fopen("data/rook.bb", "r");
    fread(rook_look_up_table, sizeof(BitBoard), 64, in);
    fclose(in);
}
