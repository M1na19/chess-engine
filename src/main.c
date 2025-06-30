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
  apply_move(cp, (Move){.move_type = CAPTURE,
                        .capture = {.from = from_position("E2"),
                                    .to = from_position("E6")}});
  print_position(cp);

  Vector v = alloca(sizeof(struct vector));
  init_vector(v, sizeof(Move), 256);

  cp->side_to_move = WHITE;
  gen_pseudo_legal_moves(cp, v);

  for (int i = 0; i < v->count; i++) {
    ChessPosition c_cp = alloca(sizeof(struct chess_position));
    memcpy(c_cp, cp, sizeof(struct chess_position));
    apply_move(c_cp, VALUE(Move, get_vector(v, i)));
    print_position(c_cp);
  }

  free_vector(v);
}
