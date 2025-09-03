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

  ChessPosition cp = engine_init_position();

  printf("%lld", engine_perft(&cp, 6, 0));
}
