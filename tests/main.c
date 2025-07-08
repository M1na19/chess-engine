#include <string.h>

#include "engine/debug.h"
#include "engine/precompute/load.h"
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
  init_position(cp);

  int depth = 6;
  char **out = malloc(sizeof(char *) * depth);
  for (int i = 0; i < depth; i++)
    out[i] = malloc(sizeof(char) * 6);
  printf("%ld", perft(cp, depth, 0, out));
}
