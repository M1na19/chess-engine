#include "engine/precompute/pieces/slide.h"
#include <alloca.h>
#include <stdlib.h>
#include <string.h>
#if defined(__APPLE__)
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

void *magic_worker(void *arg) {
  MagicThreadArgs *args = (MagicThreadArgs *)arg;
  Vector values = args->values;

  uint8_t nr_bits = atomic_load(args->nr_bits);
  uint32_t table_size = 1 << nr_bits;
  BitBoard *used = calloc(table_size, sizeof(BitBoard));

  for (int tries = 0; tries < NR_MAX_TRIES / THREADS_PER_SQUARE; tries++) {
    if (nr_bits != atomic_load(args->nr_bits)) {
      tries = 0;
      nr_bits = atomic_load(args->nr_bits);
    }
    table_size = 1 << nr_bits;
    memset(used, 0, sizeof(BitBoard) * table_size);

    uint64_t m = random_magic();

    // Check magic numbers against values
    uint8_t fail = 0;
    for (size_t i = 0; i < values->count; i++) {
      OccupancyAttack oa = VALUE(OccupancyAttack, get_vector(values, i));
      uint64_t key = (oa.occupancy * m) >> (64 - nr_bits);

      if (used[key] == 0) {
        used[key] = oa.attack;
      } else if (used[key] != oa.attack) {
        fail = 1;
        break;
      }
    }

    // Win case
    if (!fail) {
      pthread_mutex_lock(args->result_mutex);

      // Check other threads finished already
      if (nr_bits == atomic_load(args->nr_bits) && nr_bits > 0) {
        printf("\tFound magic with %u bits\n", nr_bits);
        atomic_store(args->magic_number_found, m);
        atomic_store(args->nr_bits, nr_bits - 1);
        memcpy(args->result_table, used, table_size * sizeof(BitBoard));
      }

      // Release
      pthread_mutex_unlock(args->result_mutex);
    }
  }
  free(used);
  return NULL;
}

void generate_bishop_bitboards() {
  FILE *out = fopen("./data/bishop.bb", "wb");

  const int8_t dir[4] = {9, 7, -9, -7};
  const int8_t file_delta[4] = {1, -1, -1, 1};

  // Vector of vectors, keeps for each square the occupancy + attack combination
  Vector data = alloca(sizeof(struct vector));
  init_vector(data, sizeof(struct vector), 64);

  // Vector to keep possible moves from specific square
  Vector moves = alloca(sizeof(struct vector));
  init_vector(moves, sizeof(uint8_t), 64);

  for (uint8_t sq = 0; sq < 64; sq++) {
    printf("Calculating all attack patterns for square %u\n", sq);

    uint8_t file = sq % 8;
    for (int k = 0; k < 4; k++) {
      int8_t file_moved = file + file_delta[k];
      int8_t sq_moved = sq + dir[k];

      // ignore edge cases, check square is inside table
      while (sq_moved >= 0 && sq_moved <= 63 && file_moved >= 0 &&
             file_moved <= 7) {
        push_vector(moves, &sq_moved);
        sq_moved += dir[k];
        file_moved += file_delta[k];
      }
    }

    // Vector to keep occupancy + attack combinations
    Vector oa_vec = alloca(sizeof(struct vector));
    init_vector(oa_vec, sizeof(OccupancyAttack), 1ULL << moves->count);

    // Go through all combinations
    for (BitBoard comb = 0; comb < 1ULL << moves->count; comb++) {
      BitBoard comb_c = comb;
      BitBoard oc = 0;

      // write combination to bitboard
      for (int i = 0; comb_c; i++, comb_c >>= 1) {
        if (comb_c & 1) {
          oc |= 1ULL << VALUE(uint8_t, get_vector(moves, i));
        }
      }

      // Calculate attack based on current occupancy
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
        if (sq_moved >= 0 && sq_moved <= 63 && file_moved >= 0 &&
            file_moved <= 7) {
          attack |= 1ULL << sq_moved;
        }
      }
      push_vector(oa_vec,
                  &(OccupancyAttack){.occupancy = oc, .attack = attack});
    }
    push_vector(data, oa_vec);

    // Reset moves
    moves->count = 0;
  }
  free_vector(moves);

  // Calculating multithreaded magic numbers
  pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

  for (uint8_t sq = 0; sq < 64; sq++) {
    printf("Searching magic for square %u\n", sq);

    Vector values = get_vector(data, sq);

    // Calculate minimum number of bits
    atomic_char nr_bits = 14;

    uint32_t table_size = 1UL << nr_bits;
    pthread_t threads[THREADS_PER_SQUARE];
    MagicThreadArgs thread_args[THREADS_PER_SQUARE];

    // Keep attack vector (encoded with magic num)
    BitBoard *result_table = malloc(table_size * sizeof(BitBoard));

    atomic_ullong magic_num = 0;
    for (int i = 0; i < THREADS_PER_SQUARE; i++) {
      thread_args[i] = (MagicThreadArgs){.values = values,
                                         .nr_bits = &nr_bits,
                                         .result_table = result_table,
                                         .magic_number_found = &magic_num,
                                         .result_mutex = &result_mutex};
      pthread_create(&threads[i], NULL, magic_worker, &thread_args[i]);
    }

    for (int i = 0; i < THREADS_PER_SQUARE; i++) {
      pthread_join(threads[i], NULL);
    }
    uint64_t m = atomic_load(&magic_num);
    uint8_t bits =
        atomic_load(&nr_bits) + 1; // Last try failed so best try has one more
    if (magic_num != 0) {
      printf("✓ Found magic for square %u with %u bits: %lu\n", sq, bits, m);
      fwrite(&m, sizeof(uint64_t), 1, out);
      fwrite(&bits, sizeof(nr_bits), 1, out);
      fwrite(result_table, sizeof(BitBoard), table_size, out);
    } else {
      printf("✗ Failed for square %u\n", sq);
      // Do this till you find one with at least max bits
      sq--;
    }

    free(result_table);
  }
  free_vector(data);
  fclose(out);
}

void generate_rook_bitboards() {
  const int8_t dir[4] = {8, -8, 1, -1};
  const int8_t file_delta[4] = {0, 0, 1, -1};

  FILE *out = fopen("./data/rook.bb", "wb");

  // Vector of vectors, keeps for each square the occupancy + attack combination
  Vector data = alloca(sizeof(struct vector));
  init_vector(data, sizeof(struct vector), 64);

  // Vector to keep possible moves from specific square
  Vector moves = alloca(sizeof(struct vector));
  init_vector(moves, sizeof(uint8_t), 64);

  for (uint8_t sq = 0; sq < 64; sq++) {
    printf("Calculating all attack patterns for square %u\n", sq);

    uint8_t file = sq % 8;
    for (int k = 0; k < 4; k++) {
      int8_t file_moved = file + file_delta[k];
      int8_t sq_moved = sq + dir[k];

      // check square is inside table
      while (sq_moved >= 0 && sq_moved <= 63 && file_moved >= 0 &&
             file_moved <= 7) {
        push_vector(moves, &sq_moved);
        sq_moved += dir[k];
        file_moved += file_delta[k];
      }
    }

    // Vector to keep occupancy + attack combinations
    Vector oa_vec = alloca(sizeof(struct vector));
    init_vector(oa_vec, sizeof(OccupancyAttack), 1ULL << moves->count);

    // Go through all combinations
    for (BitBoard comb = 0; comb < 1ULL << moves->count; comb++) {
      BitBoard comb_c = comb;
      BitBoard oc = 0;

      // write combination to bitboard
      for (int i = 0; comb_c; i++, comb_c >>= 1) {
        if (comb_c & 1) {
          oc |= 1ULL << VALUE(uint8_t, get_vector(moves, i));
        }
      }

      // Calculate attack based on current occupancy
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
        if (sq_moved >= 0 && sq_moved <= 63 && file_moved >= 0 &&
            file_moved <= 7) {
          attack |= 1ULL << sq_moved;
        }
      }
      push_vector(oa_vec,
                  &(OccupancyAttack){.occupancy = oc, .attack = attack});
    }
    push_vector(data, oa_vec);

    // Reset moves
    moves->count = 0;
  }
  free_vector(moves);
  // Calculating multithreaded magic numbers
  pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

  for (uint8_t sq = 0; sq < 64; sq++) {
    printf("Searching magic for square %u\n", sq);

    Vector values = get_vector(data, sq);

    // Calculate minimum number of bits
    atomic_char nr_bits = 15;

    uint32_t table_size = 1UL << nr_bits;
    pthread_t threads[THREADS_PER_SQUARE];
    MagicThreadArgs thread_args[THREADS_PER_SQUARE];

    // Keep attack vector (encoded with magic num)
    BitBoard *result_table = malloc(table_size * sizeof(BitBoard));

    atomic_ullong magic_num = 0;
    for (int i = 0; i < THREADS_PER_SQUARE; i++) {
      thread_args[i] = (MagicThreadArgs){.values = values,
                                         .nr_bits = &nr_bits,
                                         .result_table = result_table,
                                         .magic_number_found = &magic_num,
                                         .result_mutex = &result_mutex};
      pthread_create(&threads[i], NULL, magic_worker, &thread_args[i]);
    }

    for (int i = 0; i < THREADS_PER_SQUARE; i++) {
      pthread_join(threads[i], NULL);
    }
    uint64_t m = atomic_load(&magic_num);
    uint8_t bits =
        atomic_load(&nr_bits) + 1; // Last try failed so best try has one more
    if (magic_num != 0) {
      printf("✓ Found magic for square %u with %u bits: %lu\n", sq, bits, m);
      fwrite(&m, sizeof(uint64_t), 1, out);
      fwrite(&bits, sizeof(nr_bits), 1, out);
      fwrite(result_table, sizeof(BitBoard), table_size, out);
    } else {
      printf("✗ Failed for square %u\n", sq);
      // Do this till you find one with at least max bits
      sq--;
    }

    free(result_table);
  }
  free_vector(data);
  fclose(out);
}