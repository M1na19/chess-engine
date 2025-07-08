#pragma once
#define BitBoard uint64_t
#include <stdint.h>

#define NR_PROMOTION_TYPE 4
#define NR_PIECE_TYPES 6
#define NR_PIECE_COLORS 2

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
  unsigned from : 6;
  unsigned to : 6;
  unsigned is_en_passant : 1;
} Capture;

typedef enum promotion_type {
  PROMOTE_BISHOP = 0,
  PROMOTE_KNIGHT = 1,
  PROMOTE_ROOK = 2,
  PROMOTE_QUEEN = 3
} PromotionType;

typedef struct promotion {
  unsigned from : 6;
  unsigned to : 6;
  PromotionType promotion_type : 3;
} Promotion;

typedef enum castle_rights {
  CANNOT_CASTLE = 0,
  CAN_KING_SIDE_CASTLE = 1,
  CAN_QUEEN_SIDE_CASTLE = 2,
  CAN_CASTLE_BOTH_WAYS = 3
} CastleRights;

typedef enum castle { CASTLE_KING, CASTLE_QUEEN } Castle;
typedef struct en_passant {
  enum { EN_PASSANT_POSSIBLE, EN_PASSANT_NOT_POSSIBLE } en_passant_status : 1;
  uint8_t en_passant_square : 6;
} EnPassant;
typedef struct move {
  enum { CASTLE, PROMOTION, CAPTURE } move_type : 2;

  union {
    Castle castle;
    Promotion promotion;
    Capture capture;
  };
} Move;

typedef struct _undo_move {
  struct undo_piece {
    uint8_t on_table : 1;
    union {
      uint8_t sq : 6;
      PieceType piece : 3;
    } from;
    uint8_t is_promotion : 1;
    uint8_t to : 6;

  } changes[2];
  uint8_t nr_changes : 1;

  EnPassant prev_en_passant;
  uint8_t prev_half_move : 6;
  CastleRights castle_laws[NR_PIECE_COLORS];
} UndoMove;
