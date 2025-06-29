#include "engine/debug.h"
#include <stdio.h>
const char piece_symbols[2][7] = {
    {'P', 'N', 'B', 'R', 'Q', 'K', '.'}, // WHITE
    {'p', 'n', 'b', 'r', 'q', 'k', '.'} // BLACK
};

void print_position(const ChessPosition cp) {
    for (int rank = 7; rank >= 0; rank--) {
        printf("%d ", rank + 1);
        for (int file = 0; file < 8; file++) {
            const int sq = rank * 8 + file;
            printf("%c ", piece_symbols[cp->piece_board[sq].color][cp->piece_board[sq].piece]);
        }
        printf("\n");
    }
    printf("  a b c d e f g h\n");
}

uint8_t from_position(const char *pos) {
    char file = pos[0];
    char rank = pos[1];

    return (rank - '1') * 8 + file - 'A';
}
