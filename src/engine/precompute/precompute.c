#include <stdio.h>
#include <engine/engine.h>

void generate_king_bitboards() {
    const int8_t dir[8] = {7, -1, -9, 8, -8, 9, 1, -7};
    const int8_t file_delta[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

    FILE *out = fopen("data/king.bb", "w");
    for (uint8_t sq = 0; sq < 64; sq++) {
        uint8_t file = sq % 8;
        BitBoard bb = 0;
        for (int k = 0; k < 8; k++) {
            int8_t file_moved = file + file_delta[k];
            int8_t sq_moved = sq + dir[k];
            if (sq_moved >= 0 && sq_moved <= 63 && file_moved >= 0 && file_moved <= 7) {
                bb |= 1ULL << sq_moved;
            }
        }
        fwrite(&bb, sizeof(BitBoard), 1, out);
    }
    fclose(out);
}

void generate_knight_bitboards() {
    const int8_t dir[8] = {6, 15, 17, 10, -6, -15, -17, -10};
    const int8_t file_delta[8] = {-2, -1, 1, 2, 2, 1, -1, -2};

    FILE *out = fopen("data/knight.bb", "w");
    for (uint8_t sq = 0; sq < 64; sq++) {
        uint8_t file = sq % 8;
        BitBoard bb = 0;
        for (int k = 0; k < 8; k++) {
            int8_t file_moved = file + file_delta[k];
            int8_t sq_moved = sq + dir[k];
            if (sq_moved >= 0 && sq_moved <= 63 && file_moved >= 0 && file_moved <= 7) {
                bb |= 1ULL << sq_moved;
            }
        }
        fwrite(&bb, sizeof(BitBoard), 1, out);
    }
    fclose(out);
}

int main() {
    generate_king_bitboards();
    generate_knight_bitboards();
}
