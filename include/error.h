#pragma once

#include <stdio.h>

void *f_malloc(size_t size);

void *f_realloc(void *original, size_t size);

void assert_unreachable(const char *msg);
