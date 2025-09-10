#pragma once

#include "vector.h"
#include <stdint.h>

#define NR_PROMOTION_TYPE 4
#define NR_PIECE_TYPES 6
#define NR_PIECE_COLORS 2

typedef uint64_t BitBoard;
typedef Vector VectorBitboard;

typedef enum color {
  WHITE = 0,
  BLACK = 1,
} Color;

#define ENEMY_COLOR(a) (1 - (a))
typedef enum piece_type {
  PAWN = 0,
  KNIGHT = 1,
  BISHOP = 2,
  ROOK = 3,
  QUEEN = 4,
  KING = 5,
  NONE = 6,
} PieceType;

typedef struct capture {
  unsigned from;
  unsigned to;
  unsigned is_en_passant;
} Capture;

typedef enum promotion_type {
  PROMOTE_BISHOP = 0,
  PROMOTE_KNIGHT = 1,
  PROMOTE_ROOK = 2,
  PROMOTE_QUEEN = 3
} PromotionType;

typedef struct promotion {
  unsigned from;
  unsigned to;
  PromotionType promotion_type;
} Promotion;

typedef enum castle_rights {
  CANNOT_CASTLE = 0,
  CAN_KING_SIDE_CASTLE = 1,
  CAN_QUEEN_SIDE_CASTLE = 2,
  CAN_CASTLE_BOTH_WAYS = 3
} CastleRights;

typedef enum castle { CASTLE_KING, CASTLE_QUEEN } Castle;
typedef struct en_passant {
  enum { EN_PASSANT_POSSIBLE, EN_PASSANT_NOT_POSSIBLE } en_passant_status;
  uint8_t en_passant_square;
} EnPassant;
typedef struct move {
  enum { CASTLE, PROMOTION, CAPTURE } move_type;

  union {
    Castle castle;
    Promotion promotion;
    Capture capture;
  };
} Move;
typedef Vector VectorMove;

typedef struct _undo_move {
  struct undo_piece {
    uint8_t on_table;
    union {
      uint8_t sq;
      PieceType piece;
    } from;
    uint8_t is_promotion;
    uint8_t to;

  } changes[2];
  uint8_t nr_changes;

  EnPassant prev_en_passant;
  uint8_t prev_half_move;
  CastleRights castle_laws[NR_PIECE_COLORS];
} UndoMove;
