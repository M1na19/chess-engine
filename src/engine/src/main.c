#include "debug.h"
#include "precompute/load.h"
#include "stdlib.h"
#include "vector.h"
#include <alloca.h>

int main() {
  // Load precomputed
  load_king_bb();
  load_knight_bb();
  load_bishop_bb();
  load_rook_bb();

  ChessPosition cp = alloca(sizeof(struct chess_position));
  char *fen = malloc(sizeof(char) * 256);
  int depth;
  scanf("%[^\n] %d", fen, &depth);
  init_position_from_fen(cp, fen);

  printf("%lld", perft(cp, depth, 0));
}
