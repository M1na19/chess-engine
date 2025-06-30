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

#if defined(__APPLE__)
#include <stdlib.h>

uint64_t random_magic() {
  uint64_t val;
  arc4random_buf(&val, sizeof(val));
  return val | 1;
}

#elif defined(__linux__)
#include <fcntl.h>
#include <unistd.h>

uint64_t random_magic() {
  uint64_t val;
  int fd = open("/dev/urandom", O_RDONLY);

  read(fd, &val, sizeof(val));
  close(fd);
  return val | 1;
}
#endif

void generate_king_bitboards() {
  const int8_t dir[8] = {7, -1, -9, 8, -8, 9, 1, -7};
  const int8_t file_delta[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

  FILE *out = fopen("data/king.bb", "w");
  for (uint8_t sq = 0; sq < 64; sq++) {
    uint8_t file = sq % 8;
    BitBoard bb = 0;
    for (int k = 0; k < 8; k++) {
      int8_t file_moved = file + file_delta[k];
      int8_t sq_moved = sq + dir[k];
      if (sq_moved >= 0 && sq_moved <= 63 && file_moved >= 0 &&
          file_moved <= 7) {
        bb |= 1ULL << sq_moved;
      }
    }
    fwrite(&bb, sizeof(BitBoard), 1, out);
  }
  fclose(out);
}

void generate_knight_bitboards() {
  const int8_t dir[8] = {6, 15, 17, 10, -6, -15, -17, -10};
  const int8_t file_delta[8] = {-2, -1, 1, 2, 2, 1, -1, -2};

  FILE *out = fopen("data/knight.bb", "w");
  for (uint8_t sq = 0; sq < 64; sq++) {
    uint8_t file = sq % 8;
    BitBoard bb = 0;
    for (int k = 0; k < 8; k++) {
      int8_t file_moved = file + file_delta[k];
      int8_t sq_moved = sq + dir[k];
      if (sq_moved >= 0 && sq_moved <= 63 && file_moved >= 0 &&
          file_moved <= 7) {
        bb |= 1ULL << sq_moved;
      }
    }
    fwrite(&bb, sizeof(BitBoard), 1, out);
  }
  fclose(out);
}

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

#define THREADS_PER_SQUARE 16

void *magic_worker(void *arg) {
  MagicThreadArgs *args = (MagicThreadArgs *)arg;
  Vector values = args->values;

  uint32_t table_size = 1 << args->nr_bits;
  while (atomic_load(args->magic_number_found) == 0) {
    uint64_t m = random_magic();
    BitBoard *used = calloc(table_size, sizeof(BitBoard));

    uint8_t fail = 0;
    for (size_t i = 0; i < values->count; i++) {
      OccupancyAttack oa = VALUE(OccupancyAttack, get_vector(values, i));
      uint64_t key = (oa.occupancy * m) >> (64 - args->nr_bits);

      if (used[key] == 0) {
        used[key] = oa.attack;
      } else if (used[key] != oa.attack) {
        fail = 1;
        break;
      }
    }

    if (!fail) {
      pthread_mutex_lock(args->result_mutex);
      if (*(args->magic_number_found) == 0) {
        atomic_store(args->magic_number_found, m);
        memcpy(args->result_table, used, table_size * sizeof(BitBoard));
      }
      pthread_mutex_unlock(args->result_mutex);
    }

    free(used);
  }

  return NULL;
}

void generate_bishop_bitboards() {
  FILE *out = fopen("data/bishop.bb", "w");

  const int8_t dir[4] = {9, 7, -9, -7};
  const int8_t file_delta[4] = {1, -1, -1, 1};

  Vector data = alloca(sizeof(struct vector));
  init_vector(data, sizeof(struct vector), 64);

  Vector moves = alloca(sizeof(struct vector));
  init_vector(moves, sizeof(uint8_t), 64);
  for (uint8_t sq = 0; sq < 64; sq++) {
    printf("Calculating all attack patterns for square %u\n", sq);
    uint8_t file = sq % 8;
    for (int k = 0; k < 4; k++) {
      int8_t file_moved = file + file_delta[k];
      int8_t sq_moved = sq + dir[k];

      // ignore edge cases
      while (sq_moved >= 8 && sq_moved <= 55 && file_moved >= 1 &&
             file_moved <= 6) {
        push_vector(moves, &sq_moved);
        sq_moved += dir[k];
        file_moved += file_delta[k];
      }
    }

    Vector oa_vec = alloca(sizeof(struct vector));
    init_vector(oa_vec, sizeof(OccupancyAttack), 1ULL << moves->count);
    for (BitBoard comb = 0; comb < 1ULL << moves->count; comb++) {
      BitBoard comb_c = comb;
      BitBoard oc = 0;

      // write combination to bitboard
      for (int i = 0; comb_c; i++, comb_c >>= 1) {
        if (comb_c & 1) {
          oc |= 1ULL << VALUE(uint8_t, get_vector(moves, i));
        }
      }
      BitBoard attack = 0;
      for (int k = 0; k < 4; k++) {
        int8_t file_moved = file + file_delta[k];
        int8_t sq_moved = sq + dir[k];
        while (sq_moved >= 0 && sq_moved <= 63 && file_moved >= 0 &&
               file_moved <= 7 && (oc & 1ULL << sq_moved) == 0) {
          attack |= 1ULL << sq_moved;
          sq_moved += dir[k];
          file_moved += file_delta[k];
        }

        // if blocked by piece, add capture
        if (sq_moved >= 8 && sq_moved <= 55 && file_moved >= 1 &&
            file_moved <= 6) {
          attack |= 1ULL << sq_moved;
        }
      }
      push_vector(oa_vec,
                  &(OccupancyAttack){.occupancy = oc, .attack = attack});
    }
    push_vector(data, oa_vec);
    moves->count = 0;
  }

  // Now i have a vector type data[nr_squares][idx_occupancy] and i need to find
  // magic number for each square

  pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
  BitBoard *used_table =
      malloc((1UL << 12) * sizeof(BitBoard)); // temporary buffer

  for (uint8_t sq = 0; sq < 64; sq++) {
    printf("Searching magic for square %u\n", sq);

    Vector values = get_vector(data, sq);
    uint8_t nr_bits = 0;
    uint64_t tmp = values->count - 1;
    while (tmp) {
      nr_bits++;
      tmp >>= 1;
    }

    uint32_t table_size = 1UL << nr_bits;
    pthread_t threads[THREADS_PER_SQUARE];
    MagicThreadArgs thread_args[THREADS_PER_SQUARE];

    BitBoard *result_table = malloc(table_size * sizeof(BitBoard));

    atomic_ullong magic_num = 0;
    for (int i = 0; i < THREADS_PER_SQUARE; i++) {
      thread_args[i] = (MagicThreadArgs){.values = values,
                                         .nr_bits = nr_bits,
                                         .result_table = result_table,
                                         .magic_number_found = &magic_num,
                                         .result_mutex = &result_mutex};
      pthread_create(&threads[i], NULL, magic_worker, &thread_args[i]);
    }

    for (int i = 0; i < THREADS_PER_SQUARE; i++) {
      pthread_join(threads[i], NULL);
    }
    uint64_t m = atomic_load(&magic_num);

    if (magic_num != 0) {
      printf("✓ Found magic for square %u: %llu\n", sq, m);
      fwrite(&sq, sizeof(sq), 1, out);
      fwrite(&m, sizeof(uint64_t), 1, out);
      fwrite(&table_size, sizeof(table_size), 1, out);
      fwrite(result_table, sizeof(BitBoard), table_size, out);
    } else {
      printf("✗ Failed for square %u\n", sq);
    }

    free(result_table);
  }
  free(used_table);

  fclose(out);
}

void generate_rook_bitboards() {
  const int8_t dir[4] = {8, -8, 1, -1};
  const int8_t file_delta[4] = {0, 0, 1, -1};

  FILE *out = fopen("data/rook.bb", "w");

  Vector data = alloca(sizeof(struct vector));
  init_vector(data, sizeof(struct vector), 64);

  Vector moves = alloca(sizeof(struct vector));
  init_vector(moves, sizeof(uint8_t), 64);
  for (uint8_t sq = 0; sq < 64; sq++) {
    printf("Calculating all attack patterns for square %u\n", sq);
    uint8_t file = sq % 8;
    for (int k = 0; k < 4; k++) {
      int8_t file_moved = file + file_delta[k];
      int8_t sq_moved = sq + dir[k];

      // ignore edge cases
      while (sq_moved >= 8 && sq_moved <= 55 && file_moved >= 1 &&
             file_moved <= 6) {
        push_vector(moves, &sq_moved);
        sq_moved += dir[k];
        file_moved += file_delta[k];
      }
    }

    Vector oa_vec = alloca(sizeof(struct vector));
    init_vector(oa_vec, sizeof(OccupancyAttack), 1ULL << moves->count);
    for (BitBoard comb = 0; comb < 1ULL << moves->count; comb++) {
      BitBoard comb_c = comb;
      BitBoard oc = 0;

      // write combination to bitboard
      for (int i = 0; comb_c; i++, comb_c >>= 1) {
        if (comb_c & 1) {
          oc |= 1ULL << VALUE(uint8_t, get_vector(moves, i));
        }
      }
      BitBoard attack = 0;
      for (int k = 0; k < 4; k++) {
        int8_t file_moved = file + file_delta[k];
        int8_t sq_moved = sq + dir[k];
        while (sq_moved >= 0 && sq_moved <= 63 && file_moved >= 0 &&
               file_moved <= 7 && (oc & 1ULL << sq_moved) == 0) {
          attack |= 1ULL << sq_moved;
          sq_moved += dir[k];
          file_moved += file_delta[k];
        }

        // if blocked by piece, add capture
        if (sq_moved >= 8 && sq_moved <= 55 && file_moved >= 1 &&
            file_moved <= 6) {
          attack |= 1ULL << sq_moved;
        }
      }
      push_vector(oa_vec,
                  &(OccupancyAttack){.occupancy = oc, .attack = attack});
    }
    push_vector(data, oa_vec);
    moves->count = 0;
  }

  // Now i have a vector type data[nr_squares][idx_occupancy] and i need to find
  // magic number for each square
  pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
  BitBoard *used_table =
      malloc((1UL << 12) * sizeof(BitBoard)); // temporary buffer

  for (uint8_t sq = 0; sq < 64; sq++) {
    printf("Searching magic for square %u\n", sq);

    Vector values = get_vector(data, sq);
    uint8_t nr_bits = 0;
    uint64_t tmp = values->count - 1;
    while (tmp) {
      nr_bits++;
      tmp >>= 1;
    }

    uint32_t table_size = 1UL << nr_bits;
    pthread_t threads[THREADS_PER_SQUARE];
    MagicThreadArgs thread_args[THREADS_PER_SQUARE];

    BitBoard *result_table = malloc(table_size * sizeof(BitBoard));

    atomic_ullong magic_num = 0;
    for (int i = 0; i < THREADS_PER_SQUARE; i++) {
      thread_args[i] = (MagicThreadArgs){.values = values,
                                         .nr_bits = nr_bits,
                                         .result_table = result_table,
                                         .magic_number_found = &magic_num,
                                         .result_mutex = &result_mutex};
      pthread_create(&threads[i], NULL, magic_worker, &thread_args[i]);
    }

    for (int i = 0; i < THREADS_PER_SQUARE; i++) {
      pthread_join(threads[i], NULL);
    }
    uint64_t m = atomic_load(&magic_num);

    if (magic_num != 0) {
      printf("✓ Found magic for square %u: %llu\n", sq, m);
      fwrite(&sq, sizeof(sq), 1, out);
      fwrite(&m, sizeof(uint64_t), 1, out);
      fwrite(&table_size, sizeof(table_size), 1, out);
      fwrite(result_table, sizeof(BitBoard), table_size, out);
    } else {
      printf("✗ Failed for square %u\n", sq);
    }

    free(result_table);
  }
  free(used_table);

  fclose(out);
}

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
