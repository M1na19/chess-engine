#include "engine/precompute/pieces/slide.h"
#include "error.h"
#include <alloca.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#if defined(__APPLE__)
uint64_t random_magic() {
  uint64_t val;
  arc4random_buf(&val, sizeof(val));
  return val;
}

#elif defined(__linux__)
#include <fcntl.h>
#include <unistd.h>

uint64_t random_magic() {
  uint64_t val;
  int fd = open("/dev/urandom", O_RDONLY);

  read(fd, &val, sizeof(val));
  close(fd);
  return val;
}
#endif

atomic_char cancelation = 0;
sem_t semaphore;
void handle_sigint(int sig) {
  (void)sig;
  printf("\nCaught Ctrl+C! Closing up...\n");
  atomic_store(&cancelation, 1);
}
void *magic_worker(void *arg) {
  MagicThreadArgs *args = (MagicThreadArgs *)arg;
  Vector values = args->values;

  uint8_t nr_bits = atomic_load(args->nr_bits);
  uint32_t table_size = 1 << nr_bits;
  BitBoard *used = calloc(table_size, sizeof(BitBoard));

  uint8_t found = 0;
  sem_wait(&semaphore);
  while (atomic_load(&cancelation) == 0) {
    if (found == 1) {
      sem_wait(&semaphore);
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
      atomic_store(args->magic_number_found, m);
      atomic_store(args->nr_bits, nr_bits);
      memcpy(args->result_table, used, table_size * sizeof(BitBoard));
      nr_bits--;
      found = 1;
    }
    if (found == 1) {
      sem_post(&semaphore);
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

  // Generating magic numbers
  pthread_t threads[64];

  BitBoard **result_table = f_malloc(64 * sizeof(BitBoard *));

  atomic_ullong *magic_nums = f_malloc(64 * sizeof(atomic_ullong));
  memset(magic_nums, 0, sizeof(uint64_t) * 64);

  atomic_char *nr_bits = f_malloc(64 * sizeof(atomic_char));
  for (uint8_t sq = 0; sq < 64; sq++) {

    Vector values = get_vector(data, sq);

    // Calculate minimum number of bits
    nr_bits[sq] = 16;

    uint32_t table_size = 1UL << nr_bits[sq];

    // Keep attack vector (encoded with magic num)
    result_table[sq] = f_malloc(table_size * sizeof(BitBoard));

    MagicThreadArgs *args = alloca(sizeof(MagicThreadArgs));
    *args = (MagicThreadArgs){
        .values = values,
        .nr_bits = nr_bits + sq,
        .result_table = result_table[sq],
        .magic_number_found = magic_nums + sq,
    };
    pthread_create(&threads[sq], NULL, magic_worker, args);
  }

  // Debug
  while (atomic_load(&cancelation) == 0) {
    uint64_t size = 0;
    uint8_t all_found = 0;
    for (int i = 0; i < 64; i++) {
      all_found += (atomic_load(&magic_nums[i]) != 0);
      size = size + (1ULL << atomic_load(&nr_bits[i]));
    }
    printf("\033[2J\033[H");
    printf("Found %d/64\n", all_found);
    printf("Current size: %lluB / %llu KB\n", size, size >> 10);
    sleep(1);
  }

  for (int i = 0; i < 64; i++) {
    pthread_join(threads[i], NULL);
    fwrite(&magic_nums[i], sizeof(uint64_t), 1, out);
    fwrite(&nr_bits[i], sizeof(uint8_t), 1, out);

    uint32_t table_size = 1 << nr_bits[i];
    fwrite(result_table[i], sizeof(BitBoard), table_size, out);
    free(result_table[i]);
  }

  // Allow rook calculation
  cancelation = 0;
  free(result_table);
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

  // Generating magic numbers
  pthread_t threads[64];

  BitBoard **result_table = f_malloc(64 * sizeof(BitBoard *));

  atomic_ullong *magic_nums = f_malloc(64 * sizeof(atomic_ullong));
  memset(magic_nums, 0, sizeof(uint64_t) * 64);

  atomic_char *nr_bits = f_malloc(64 * sizeof(atomic_char));
  for (uint8_t sq = 0; sq < 64; sq++) {

    Vector values = get_vector(data, sq);

    // Calculate minimum number of bits
    nr_bits[sq] = 14;

    uint32_t table_size = 1UL << nr_bits[sq];

    // Keep attack vector (encoded with magic num)
    result_table[sq] = f_malloc(table_size * sizeof(BitBoard));

    MagicThreadArgs *args = alloca(sizeof(MagicThreadArgs));
    *args = (MagicThreadArgs){
        .values = values,
        .nr_bits = nr_bits + sq,
        .result_table = result_table[sq],
        .magic_number_found = magic_nums + sq,
    };
    pthread_create(&threads[sq], NULL, magic_worker, args);
  }
  // Debug
  while (atomic_load(&cancelation) == 0) {
    uint64_t size = 0;
    uint8_t all_found = 0;
    for (int i = 0; i < 64; i++) {
      all_found += (atomic_load(&magic_nums[i]) != 0);
      size = size + (1ULL << atomic_load(&nr_bits[i]));
    }
    printf("\033[2J\033[H");
    printf("Found %d/64\n", all_found);
    printf("Current size: %lluB / %llu KB\n", size, size >> 10);
    sleep(1);
  }
  for (int i = 0; i < 64; i++) {
    pthread_join(threads[i], NULL);
    fwrite(&magic_nums[i], sizeof(uint64_t), 1, out);
    fwrite(&nr_bits[i], sizeof(uint8_t), 1, out);

    uint32_t table_size = 1 << nr_bits[i];
    fwrite(result_table[i], sizeof(BitBoard), table_size, out);
    free(result_table[i]);
  }

  free(result_table);
  free_vector(data);
  fclose(out);
}