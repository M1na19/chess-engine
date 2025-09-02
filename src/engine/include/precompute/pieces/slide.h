#pragma once
#include "engine.h"
#include "pthread.h"
#include "stdatomic.h"
#include <semaphore.h>
#include <unistd.h>

extern int THREADS;
extern sem_t semaphore;
typedef struct occupancy_attack {
  BitBoard occupancy;
  BitBoard attack;
} OccupancyAttack;
typedef Vector VectorOccupancyAttack;
typedef struct {
  Vector values;
  atomic_char *nr_bits;

  atomic_ullong *magic_number_found;
  BitBoard *result_table;
} MagicThreadArgs;
void handle_sigint(int sig);
void generate_bishop_bitboards();
void generate_rook_bitboards();