#include "vector.h"
#include <stdint.h>

#include <stdlib.h>

#include <string.h>

#include "error.h"

void init_vector(Vector v, size_t data_size, size_t init_capacity) {
  v->data_size = data_size;
  v->capacity = init_capacity;
  v->count = 0;

  v->data = f_malloc(data_size * init_capacity);
}

void *get_vector(Vector v, int idx) {
  if (v->count <= idx) {
    assert_unreachable("Vector count is lower than idx on get");
  }
  return ((uint8_t *)v->data) + (idx * v->data_size);
}
void set_vector(Vector v, int idx, void *data) {
  if (v->count <= idx) {
    assert_unreachable("Vector count is lower than idx on set");
  }
  memcpy(v->data + (idx * v->data_size), data, v->data_size);
}

void push_vector(Vector v, void *to_add) {
  if (v->count >= v->capacity) {
    v->data = f_realloc(v->data, v->data_size * v->capacity * 2);
    v->capacity *= 2;
  }
  memcpy(get_vector(v, v->count), to_add, v->data_size);
  v->count++;
}

void pop_back_vector(Vector v, void *get_data) {
  if (v->count == 0) {
    assert_unreachable("Vector count is 0");
  }
  if (get_data != NULL) {
    memcpy(get_data, get_vector(v, v->count - 1), v->data_size);
  }
  v->count--;
}
void resize(Vector v, size_t new_cap) {
  if (v->capacity < new_cap) {
    v->data = f_realloc(v->data, new_cap * v->data_size);
    v->capacity = new_cap;
  }
}
void free_vector(Vector v) { free(v->data); }
