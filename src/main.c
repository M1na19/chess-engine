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
}
