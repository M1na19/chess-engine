#include "engine/precompute/pieces/slide.h"
#include "engine/precompute/pieces/step.h"
#include <alloca.h>
#include <assert.h>
#include <engine/engine.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main() {
  printf("Generating king bitboard:\n");
  generate_king_bitboards();
  printf("Done\n");
  printf("Generating knight bitboard:\n");
  generate_knight_bitboards();
  printf("Done\n");
  printf("Generating bishop bitboard:\n");
  generate_bishop_bitboards();
  printf("Done\n");
  printf("Generating rook bitboard:\n");
  generate_rook_bitboards();
  printf("Done\n");
}
