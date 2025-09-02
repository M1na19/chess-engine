#include "vector.h"
#include <stdint.h>

#include <stdlib.h>

#include <string.h>

#include "utils.h"

Vector vector_init(size_t data_size, size_t init_capacity) {
  Vector v;
  v.data_size = data_size;
  v.capacity = init_capacity;
  v.count = 0;

  v.data = f_malloc(data_size * init_capacity);
  return v;
}

void *vector_get(Vector v, int idx) {
  return ((uint8_t *)v.data) + (idx * v.data_size);
}

void vector_push(Vector *v, void *to_add) {
  if (v->count >= v->capacity) {
    v->data = f_realloc(v->data, v->capacity * 2 * v->data_size);
    v->capacity *= 2;
  }
  memcpy(vector_get(*v, v->count), to_add, v->data_size);
  v->count++;
}

void vector_pop_back(Vector *v, void *get_data) {
  if (v->count == 0) {
    assert_unreachable("Vector count is 0");
  }

  if (get_data != NULL) {
    memcpy(get_data, vector_get(*v, v->count - 1), v->data_size);
  }
  v->count--;
}
void vector_consume(Vector *dest, Vector src) {
  if (dest->data_size != src.data_size) {
    assert_unreachable("Diffrent size vectors");
  }

  dest->capacity += src.count;
  dest->data = f_realloc(dest->data, dest->capacity * dest->data_size);
  memcpy((uint8_t *)dest->data + dest->data_size * dest->count, src.data,
         src.count * src.data_size);
  dest->count += src.count;
  vector_free(src);
}
void vector_free(Vector v) { free(v.data); }
