#pragma once
#include "moves.h"
#include "vector.h"
#include <stdint.h>
#define BitBoard uint64_t

#define NR_PIECE_COLORS 2

typedef enum color {
  WHITE = 0,
  BLACK = 1,
} Color;

#define ENEMY_COLOR(a) (((a) + 1) % 2)

typedef enum castle_rights {
  CANNOT_CASTLE = 0,
  CAN_KING_SIDE_CASTLE = 1,
  CAN_QUEEN_SIDE_CASTLE = 2,
  CAN_CASTLE_BOTH_WAYS = 3
} CastleRights;

typedef struct chess_position {
  BitBoard board[NR_PIECE_COLORS][NR_PIECE_TYPES];

  struct {
    Color color : 1;
    PieceType piece : 3;
  } piece_board[64];

  CastleRights castle_laws[NR_PIECE_COLORS];
  Color side_to_move;
  BitBoard en_passant_square;
  unsigned int half_move_count;
  unsigned int move_count;
} *ChessPosition;

void init_position(ChessPosition cp);
void init_position_from_fen(ChessPosition cp, const char *fen);

void apply_move(ChessPosition cp, Move m);

void gen_legal_moves(ChessPosition cp, Vector v);
int perft(ChessPosition cp, int depth);