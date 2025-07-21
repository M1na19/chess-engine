#include "evaluate/nn.h"
#include "error.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Random float between [0, 1)
static float rand_float() { return (float)rand() / (RAND_MAX + 1.0f); }

float sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }

float sigmoid_derivative(float x) {
  float s = sigmoid(x);
  return s * (1.0f - s);
}

float tanhf_derivative(float x) {
  float t = tanhf(x);
  return 1.0f - t * t;
}

Network init_nn(uint32_t input, uint32_t output, uint32_t hidden,
                uint32_t nr_hidden) {
  Network nn;
  nn.activation = tanhf;
  nn.activation_derivative = tanhf_derivative;
  nn.num_layers = nr_hidden + 1;
  nn.layers = f_malloc(sizeof(Layer) * nn.num_layers);

  // Initialize layers sizes
  nn.layers[0].input_size = input;
  nn.layers[0].output_size = hidden;

  for (uint32_t i = 1; i < nn.num_layers - 1; i++) {
    nn.layers[i].input_size = hidden;
    nn.layers[i].output_size = hidden;
  }

  nn.layers[nn.num_layers - 1].input_size = hidden;
  nn.layers[nn.num_layers - 1].output_size = output;

  // Allocate weights and biases and initialize with random values
  for (uint32_t i = 0; i < nn.num_layers; i++) {
    Layer *layer = &nn.layers[i];
    layer->weights = f_malloc(sizeof(float *) * layer->input_size);
    layer->biases = f_malloc(sizeof(float) * layer->output_size);
    for (uint32_t j = 0; j < layer->input_size; j++) {
      layer->weights[j] = f_malloc(sizeof(float) * layer->output_size);
    }

    float limit = sqrtf(6.0f / (layer->input_size + layer->output_size));
    for (uint32_t out_idx = 0; out_idx < layer->output_size; out_idx++) {
      for (uint32_t in_idx = 0; in_idx < layer->input_size; in_idx++) {
        layer->weights[in_idx][out_idx] = -limit + 2 * limit * rand_float();
      }
      layer->biases[out_idx] = 0.0f;
    }
  }

  return nn;
}

float *forward_pass(Network nn, float *input) {
  Vector in_vec = malloc(sizeof(struct vector));
  Vector out_vec = malloc(sizeof(struct vector));

  init_vector(in_vec, sizeof(float), nn.layers[0].input_size);
  in_vec->count = nn.layers[0].input_size;

  init_vector(out_vec, sizeof(float), nn.layers[0].output_size);
  out_vec->count = nn.layers[0].output_size;

  memcpy(in_vec->data, input, sizeof(float) * nn.layers[0].input_size);

  for (uint32_t i = 0; i < nn.num_layers; i++) {
    Layer *layer = &nn.layers[i];
    resize(in_vec, layer->input_size);
    in_vec->count = layer->input_size;
    resize(out_vec, layer->output_size);
    out_vec->count = layer->output_size;

    for (uint32_t j = 0; j < layer->output_size; j++) {
      float sum = 0.0f;
      for (uint32_t k = 0; k < layer->input_size; k++) {
        float a = VALUE(float, get_vector(in_vec, k));
        sum += layer->weights[k][j] * a;
      }
      sum += layer->biases[j];
      float activated = nn.activation(sum);
      set_vector(out_vec, j, &activated);
    }

    // Swap in_vec and out_vec pointers for next layer input
    Vector temp = in_vec;
    in_vec = out_vec;
    out_vec = temp;
  }

  // Copy result because in_vec will be freed outside or re-used
  float *result =
      malloc(sizeof(float) * nn.layers[nn.num_layers - 1].output_size);
  memcpy(result, in_vec->data,
         sizeof(float) * nn.layers[nn.num_layers - 1].output_size);

  free_vector(in_vec);
  free_vector(out_vec);
  free(in_vec);
  free(out_vec);

  return result;
}

void train_batch(Network nn, float **inputs, float **expected_outputs,
                 uint32_t batch_size, float learning_rate) {
  float ***weight_grads = malloc(nn.num_layers * sizeof(float **));
  for (uint32_t l = 0; l < nn.num_layers; l++) {
    Layer *layer = &nn.layers[l];
    weight_grads[l] = malloc(layer->input_size * sizeof(float *));
    for (uint32_t i = 0; i < layer->input_size; i++) {
      weight_grads[l][i] =
          calloc(layer->output_size, sizeof(float)); // initialized to 0.0
    }
  }
  float **bias_grads = malloc(nn.num_layers * sizeof(float *));
  for (uint32_t l = 0; l < nn.num_layers; l++) {
    Layer *layer = &nn.layers[l];
    bias_grads[l] =
        calloc(layer->output_size, sizeof(float)); // initialized to 0.0
  }

  for (uint32_t b = 0; b < batch_size; b++) {
    float *input = inputs[b];
    float *expected_output = expected_outputs[b];

    // Allocate and initialize activation, zs, and deltas (same as before)
    Vector *activations = malloc(sizeof(Vector) * (nn.num_layers + 1));
    Vector *zs = malloc(sizeof(Vector) * (nn.num_layers + 1));

    for (uint32_t i = 0; i <= nn.num_layers; i++) {
      activations[i] = malloc(sizeof(struct vector));
      zs[i] = malloc(sizeof(struct vector));
    }

    init_vector(activations[0], sizeof(float), nn.layers[0].input_size);
    activations[0]->count = nn.layers[0].input_size;
    memcpy(activations[0]->data, input,
           sizeof(float) * nn.layers[0].input_size);

    init_vector(zs[0], sizeof(float), nn.layers[0].input_size);
    zs[0]->count = nn.layers[0].input_size;
    memcpy(zs[0]->data, input, sizeof(float) * nn.layers[0].input_size);

    for (uint32_t l = 0; l < nn.num_layers; l++) {
      Layer *layer = &nn.layers[l];
      init_vector(zs[l + 1], sizeof(float), layer->output_size);
      zs[l + 1]->count = layer->output_size;

      init_vector(activations[l + 1], sizeof(float), layer->output_size);
      activations[l + 1]->count = layer->output_size;

      for (uint32_t j = 0; j < layer->output_size; j++) {
        float z = 0.0f;
        for (uint32_t i = 0; i < layer->input_size; i++) {
          float a = VALUE(float, get_vector(activations[l], i));
          z += layer->weights[i][j] * a;
        }
        z += layer->biases[j];
        set_vector(zs[l + 1], j, &z);

        float a = nn.activation(z);
        set_vector(activations[l + 1], j, &a);
      }
    }

    // Backprop
    Vector *deltas = malloc(sizeof(Vector) * nn.num_layers);
    for (uint32_t l = 0; l < nn.num_layers; l++) {
      deltas[l] = malloc(sizeof(struct vector));
      init_vector(deltas[l], sizeof(float), nn.layers[l].output_size);
      deltas[l]->count = nn.layers[l].output_size;
    }

    uint32_t last = nn.num_layers - 1;

    for (uint32_t j = 0; j < nn.layers[last].output_size; j++) {
      float a = VALUE(float, get_vector(activations[last + 1], j));
      float z = VALUE(float, get_vector(zs[last + 1], j));
      float error = a - expected_output[j];
      float delta = error * nn.activation_derivative(z);
      set_vector(deltas[last], j, &delta);
    }

    for (int32_t l = last - 1; l >= 0; l--) {
      Layer *layer = &nn.layers[l];
      Layer *next_layer = &nn.layers[l + 1];
      for (uint32_t i = 0; i < layer->output_size; i++) {
        float sum = 0.0f;
        for (uint32_t j = 0; j < next_layer->output_size; j++) {
          float w = next_layer->weights[i][j];
          float d = VALUE(float, get_vector(deltas[l + 1], j));
          sum += w * d;
        }
        float z = VALUE(float, get_vector(zs[l + 1], i));
        float delta = sum * nn.activation_derivative(z);
        set_vector(deltas[l], i, &delta);
      }
    }

    // Accumulate gradients
    for (uint32_t l = 0; l < nn.num_layers; l++) {
      Layer *layer = &nn.layers[l];
      for (uint32_t j = 0; j < layer->output_size; j++) {
        float delta = VALUE(float, get_vector(deltas[l], j));
        bias_grads[l][j] += delta;
        for (uint32_t i = 0; i < layer->input_size; i++) {
          float a = VALUE(float, get_vector(activations[l], i));
          weight_grads[l][i][j] += delta * a;
        }
      }
    }

    // Free temporary vectors
    for (uint32_t l = 0; l <= nn.num_layers; l++) {
      free_vector(activations[l]);
      free(activations[l]);
      free_vector(zs[l]);
      free(zs[l]);
    }
    free(activations);
    free(zs);

    for (uint32_t l = 0; l < nn.num_layers; l++) {
      free_vector(deltas[l]);
      free(deltas[l]);
    }
    free(deltas);
  }

  // Apply averaged gradients
  for (uint32_t l = 0; l < nn.num_layers; l++) {
    Layer *layer = &nn.layers[l];
    for (uint32_t j = 0; j < layer->output_size; j++) {
      layer->biases[j] -= (learning_rate / batch_size) * bias_grads[l][j];
      for (uint32_t i = 0; i < layer->input_size; i++) {
        layer->weights[i][j] -=
            (learning_rate / batch_size) * weight_grads[l][i][j];
      }
    }
  }

  // Free gradient accumulators
  for (uint32_t l = 0; l < nn.num_layers; l++) {
    for (uint32_t i = 0; i < nn.layers[l].input_size; i++) {
      free(weight_grads[l][i]);
    }
    free(weight_grads[l]);
    free(bias_grads[l]);
  }
  free(weight_grads);
  free(bias_grads);
}
