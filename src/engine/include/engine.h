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
} ChessPosition;

ChessPosition engine_init_position();
ChessPosition engine_init_position_from_fen(const char *fen);

UndoMove engine_apply_move(ChessPosition *cp, Move m);

void engine_gen_legal_moves(ChessPosition *cp, VectorMove *v);
uint64_t engine_perft(ChessPosition *cp, int max_depth, int depth);
void engine_undo_move(ChessPosition *cp, UndoMove u);

unsigned char engine_is_square_attacked(ChessPosition *cp, Color by,
                                        uint8_t position);