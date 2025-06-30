#pragma once
#include "engine/engine.h"
#include "pthread.h"
#include "stdatomic.h"

#define THREADS_PER_SQUARE 16

typedef struct occupancy_attack {
  BitBoard occupancy;
  BitBoard attack;
} OccupancyAttack;

typedef struct {
  Vector values;
  uint8_t nr_bits;

  atomic_ullong *magic_number_found;
  BitBoard *result_table;
  pthread_mutex_t *result_mutex;
} MagicThreadArgs;

void generate_bishop_bitboards();
void generate_rook_bitboards();