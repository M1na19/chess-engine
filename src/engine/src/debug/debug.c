#include "debug.h"
#include <stdio.h>
#include <string.h>
const char piece_symbols[2][7] = {
    {'P', 'N', 'B', 'R', 'Q', 'K', '.'}, // WHITE
    {'p', 'n', 'b', 'r', 'q', 'k', '.'}  // BLACK
};

void print_position(const ChessPosition cp) {
  for (int rank = 7; rank >= 0; rank--) {
    printf("%d ", rank + 1);
    for (int file = 0; file < 8; file++) {
      const int sq = rank * 8 + file;
      printf("%c ",
             piece_symbols[cp.piece_board[sq].color][cp.piece_board[sq].piece]);
    }
    printf("\n");
  }
  printf("  a b c d e f g h\n\n");
}

static inline uint8_t from_position(const char *pos) {
  char file = pos[0];
  char rank = pos[1];

  return (rank - '1') * 8 + file - 'A';
}

static inline void to_position(uint8_t square, char *out) {
  out[0] = 'a' + (square & 7);  // file
  out[1] = '1' + (square >> 3); // rank
  out[2] = '\0';
}

char *move_to_str(ChessPosition cp, Move m, char *out) {
  char from[3], to[3];

  switch (m.move_type) {
  case CASTLE:
    if (m.castle == CASTLE_KING)
      strcpy(out, "O-O");
    else
      strcpy(out, "O-O-O");
    break;

  case PROMOTION: {
    static const char promo_chars[] = {'b', 'n', 'r', 'q'};
    to_position(m.promotion.from, from);
    to_position(m.promotion.to, to);
    sprintf(out, "%s%s%c", from, to, promo_chars[m.promotion.promotion_type]);
    break;
  }

  case CAPTURE: {
    to_position(m.capture.from, from);
    to_position(m.capture.to, to);
    sprintf(out, "%s%s", from, to);
    break;
  }

  default:
    strcpy(out, "????");
  }

  return out;
}
