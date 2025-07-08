#pragma once
#include <stdio.h>

typedef struct vector {
  void *data;
  size_t data_size;
  size_t count;
  size_t capacity;
} *Vector;

#define VALUE(type, expr) (*(type *)(expr))

void init_vector(Vector v, size_t data_size, size_t init_capacity);

void *get_vector(Vector v, int idx);

void push_vector(Vector v, void *to_add);

void pop_back_vector(Vector v, void *get_data);

void free_vector(Vector v);
