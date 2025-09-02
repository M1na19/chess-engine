#pragma once
#include <stdio.h>

typedef struct vector {
  void *data;
  size_t data_size;
  size_t count;
  size_t capacity;
} Vector;

#define VALUE(type, expr) (*(type *)(expr))

Vector vector_init(size_t data_size, size_t init_capacity);

void *vector_get(Vector v, int idx);

void vector_push(Vector *v, void *to_add);

void vector_pop_back(Vector *v, void *get_data);

void vector_consume(Vector *dest, Vector src);

void vector_free(Vector v);