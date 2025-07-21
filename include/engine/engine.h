#pragma once
#include "moves.h"
#include "vector.h"
#include <stdint.h>

typedef struct chess_position {
  BitBoard board[NR_PIECE_COLORS][NR_PIECE_TYPES];

  struct {
    Color color : 1;
    PieceType piece : 3;
  } piece_board[64];

  CastleRights castle_laws[NR_PIECE_COLORS];
  Color side_to_move : 1;

  EnPassant en_passant;
  unsigned int half_move_count : 6;
  uint32_t move_count : 13;
} *ChessPosition;

void init_position(ChessPosition cp);
void init_position_from_fen(ChessPosition cp, const char *fen);

UndoMove apply_move(ChessPosition cp, Move m);
void undo_move(ChessPosition cp, UndoMove u);

void gen_legal_moves(ChessPosition cp, Vector v);
uint64_t perft(ChessPosition cp, int max_depth, int depth, char **out);
