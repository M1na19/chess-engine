#include "engine.h"
#include <string.h>

void init_position(ChessPosition cp) {
    // Clear everything
    memset(cp, 0, sizeof(*cp));
    for (int sq = 0; sq < 64; sq++) {
        cp->piece_board[sq].piece = NONE;
    }

    // White pieces
    cp->piece_board[0].color = WHITE;
    cp->piece_board[0].piece = ROOK;
    cp->piece_board[1].color = WHITE;
    cp->piece_board[1].piece = KNIGHT;
    cp->piece_board[2].color = WHITE;
    cp->piece_board[2].piece = BISHOP;
    cp->piece_board[3].color = WHITE;
    cp->piece_board[3].piece = QUEEN;
    cp->piece_board[4].color = WHITE;
    cp->piece_board[4].piece = KING;
    cp->piece_board[5].color = WHITE;
    cp->piece_board[5].piece = BISHOP;
    cp->piece_board[6].color = WHITE;
    cp->piece_board[6].piece = KNIGHT;
    cp->piece_board[7].color = WHITE;
    cp->piece_board[7].piece = ROOK;
    for (int i = 8; i < 16; i++) {
        cp->piece_board[i].color = WHITE;
        cp->piece_board[i].piece = PAWN;
    }

    // Black pieces
    cp->piece_board[56].color = BLACK;
    cp->piece_board[56].piece = ROOK;
    cp->piece_board[57].color = BLACK;
    cp->piece_board[57].piece = KNIGHT;
    cp->piece_board[58].color = BLACK;
    cp->piece_board[58].piece = BISHOP;
    cp->piece_board[59].color = BLACK;
    cp->piece_board[59].piece = QUEEN;
    cp->piece_board[60].color = BLACK;
    cp->piece_board[60].piece = KING;
    cp->piece_board[61].color = BLACK;
    cp->piece_board[61].piece = BISHOP;
    cp->piece_board[62].color = BLACK;
    cp->piece_board[62].piece = KNIGHT;
    cp->piece_board[63].color = BLACK;
    cp->piece_board[63].piece = ROOK;
    for (int i = 48; i < 56; i++) {
        cp->piece_board[i].color = BLACK;
        cp->piece_board[i].piece = PAWN;
    }


    // White pieces
    cp->board[WHITE][PAWN] = 0x000000000000FF00ULL; // rank 2
    cp->board[WHITE][ROOK] = 0x0000000000000081ULL; // a1, h1
    cp->board[WHITE][KNIGHT] = 0x0000000000000042ULL; // b1, g1
    cp->board[WHITE][BISHOP] = 0x0000000000000024ULL; // c1, f1
    cp->board[WHITE][QUEEN] = 0x0000000000000008ULL; // d1
    cp->board[WHITE][KING] = 0x0000000000000010ULL; // e1

    // Black pieces
    cp->board[BLACK][PAWN] = 0x00FF000000000000ULL; // rank 7
    cp->board[BLACK][ROOK] = 0x8100000000000000ULL; // a8, h8
    cp->board[BLACK][KNIGHT] = 0x4200000000000000ULL; // b8, g8
    cp->board[BLACK][BISHOP] = 0x2400000000000000ULL; // c8, f8
    cp->board[BLACK][QUEEN] = 0x0800000000000000ULL; // d8
    cp->board[BLACK][KING] = 0x1000000000000000ULL; // e8

    // Full castling rights for both sides
    cp->castle_laws[WHITE] = CAN_CASTLE_BOTH_WAYS;
    cp->castle_laws[BLACK] = CAN_CASTLE_BOTH_WAYS;

    // White to move first
    cp->side_to_move = WHITE;

    // No en passant target square at start
    cp->en_passant_square = 0ULL;

    // Clocks
    cp->half_move_count = 0;
    cp->move_count = 1;
}
