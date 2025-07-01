#pragma once
#include "engine/engine.h"
#include "pthread.h"
#include "stdatomic.h"
#include <unistd.h>

extern int THREADS_PER_SQUARE;
#define NR_MAX_TRIES ((int)1e8)

typedef struct occupancy_attack {
  BitBoard occupancy;
  BitBoard attack;
} OccupancyAttack;

typedef struct {
  Vector values;
  atomic_char *nr_bits;

  atomic_ullong *magic_number_found;
  BitBoard *result_table;
  pthread_mutex_t *result_mutex;
} MagicThreadArgs;

void generate_bishop_bitboards();
void generate_rook_bitboards();