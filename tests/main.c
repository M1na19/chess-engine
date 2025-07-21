#include <string.h>

#include "engine/debug.h"
#include "engine/precompute/load.h"
#include "evaluate/nn.h"
#include "stdlib.h"
#include "vector.h"
#include <alloca.h>
#include <math.h>
#include <time.h>
#define PI 3.14159265358979323846

int main() {
  srand(time(NULL));

  const int input_size = 2;
  const int output_size = 1;
  const int hidden_size = 8;
  const int hidden_layers = 8;
  const int nr_train = 100000;
  const int batch_size = 64;

  float initial_lr = .1f;
  float decay_rate = 0.f; // inverse time decay

  Network nn = init_nn(input_size, output_size, hidden_size, hidden_layers);

  // Allocate batch input and target arrays
  float **batch_inputs = malloc(sizeof(float *) * batch_size);
  float **batch_targets = malloc(sizeof(float *) * batch_size);
  for (int i = 0; i < batch_size; i++) {
    batch_inputs[i] = malloc(sizeof(float) * input_size);
    batch_targets[i] = malloc(sizeof(float) * output_size);
  }

  for (int iter = 1; iter <= nr_train; iter++) {
    for (int b = 0; b < batch_size; b++) {
      float x = ((float)rand() / RAND_MAX) * 2.0f * PI;
      float y = ((float)rand() / RAND_MAX) * 2.0f * PI;

      // Normalize to [-1, 1]
      batch_inputs[b][0] = (x / PI) - 1.0f;
      batch_inputs[b][1] = (y / PI) - 1.0f;

      float target = (sinf(x) + cosf(y)) / 1.4142f;
      batch_targets[b][0] = target;
    }

    // Inverse time decay
    float lr = initial_lr / (1.0f + decay_rate * iter);

    train_batch(nn, batch_inputs, batch_targets, batch_size, lr);
  }

  // Free batch buffers
  for (int i = 0; i < batch_size; i++) {
    free(batch_inputs[i]);
    free(batch_targets[i]);
  }
  free(batch_inputs);
  free(batch_targets);

  // Test
  float total_err = 0;
  int nr = 0;
  for (float x = 0.0f; x <= 2 * PI; x += PI / 8) {
    for (float y = 0.0f; y <= 2 * PI; y += PI / 8) {
      float norm_x = (x / PI) - 1.0f;
      float norm_y = (y / PI) - 1.0f;
      float in[2] = {norm_x, norm_y};
      float predicted = *forward_pass(nn, in);
      float actual = (sinf(x) + cosf(y)) / 1.4142f;
      total_err += fabsf(predicted - actual);
      nr++;
      printf("x: %.2f, y: %.2f | predicted: %.3f | actual: %.3f\n", x, y,
             predicted, actual);
    }
  }

  printf("Total err: %f, avg_err: %f\n", total_err, total_err / nr);
  return 0;
}