#include "vector.h"
#include <stdlib.h>

typedef struct {
  uint32_t input_size;
  uint32_t output_size;
  float **weights;
  float *biases;
} Layer;
typedef struct {
  float (*activation)(float);
  float (*activation_derivative)(float);
  uint32_t num_layers;
  Layer *layers;
} Network;
Network init_nn(uint32_t input, uint32_t output, uint32_t hidden,
                uint32_t nr_hidden);
float *forward_pass(Network nn, float *input);
void train_batch(Network nn, float **inputs, float **expected_outputs,
                 uint32_t batch_size, float learning_rate);