#include "debug.h"
#include "precompute/load.h"
#include "stdlib.h"
#include "utils.h"
#include "vector.h"
#include <alloca.h>

int main() {
  // Load precomputed
  load_king_bb();
  load_knight_bb();
  load_bishop_bb();
  load_rook_bb();

  int depth;
  char *fen = f_malloc(sizeof(char) * 256);
  scanf("%[^\n] %d", fen, &depth);
  ChessPosition cp = init_position_from_fen(fen);

  printf("%lld", perft(cp, depth, 0));
}
