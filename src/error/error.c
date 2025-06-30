#include "error.h"
#include <stdlib.h>

void *f_malloc(size_t size) {
  void *aux = malloc(size);
  if (aux == NULL) {
    printf("OOM error allocating size %ld\n", size);
    exit(129);
  }
  return aux;
}

void *f_realloc(void *original, size_t size) {
  void *aux = realloc(original, size);
  if (aux == NULL) {
    printf("OOM error reallocating size %ld\n", size);
    exit(129);
  }
  return aux;
}

void assert_unreachable(const char *msg) {
  printf("Program reached unreachable code: %s", msg);
  exit(1);
}
