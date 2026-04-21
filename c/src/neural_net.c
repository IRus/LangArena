#include "benchmark.h"

#define LEARNING_RATE 1.0
#define MOMENTUM 0.3
#define TRAIN_RATE 0.3

typedef struct NeuralNetNeuron NeuralNetNeuron;
typedef struct NeuralNetSynapse NeuralNetSynapse;

struct NeuralNetSynapse {
  double weight;
  double prev_weight;
  NeuralNetNeuron *source_neuron;
  NeuralNetNeuron *dest_neuron;
};

struct NeuralNetNeuron {

  NeuralNetSynapse **synapses_in;
  int synapses_in_count;
  int synapses_in_capacity;

  NeuralNetSynapse **synapses_out;
  int synapses_out_count;
  int synapses_out_capacity;

  double threshold;
  double prev_threshold;
  double error;
  double output;
};

static void neuron_update_weights(NeuralNetNeuron *neuron);

typedef struct {
  NeuralNetNeuron *neurons;
  int total_neurons;

  int *input_layer;
  int input_count;
  int *hidden_layer;
  int hidden_count;
  int *output_layer;
  int output_count;

  NeuralNetSynapse *synapses;
  int synapse_count;
  int synapse_capacity;
} NeuralNetwork;

typedef struct {
  uint32_t result_val;
  double sum_outputs;
  NeuralNetwork *xor_net;
} NeuralNetData;

static void neuron_init(NeuralNetNeuron *neuron) {

  double r = Helper_next_float(1.0);
  neuron->threshold = neuron->prev_threshold = r * 2.0 - 1.0;
  neuron->output = 0.0;
  neuron->error = 0.0;

  neuron->synapses_in_count = 0;
  neuron->synapses_in_capacity = 4;
  neuron->synapses_in =
      malloc(neuron->synapses_in_capacity * sizeof(NeuralNetSynapse *));

  neuron->synapses_out_count = 0;
  neuron->synapses_out_capacity = 4;
  neuron->synapses_out =
      malloc(neuron->synapses_out_capacity * sizeof(NeuralNetSynapse *));
}

static void neuron_add_synapse_in(NeuralNetNeuron *neuron,
                                  NeuralNetSynapse *synapse) {
  if (neuron->synapses_in_count >= neuron->synapses_in_capacity) {
    neuron->synapses_in_capacity *= 2;
    neuron->synapses_in =
        realloc(neuron->synapses_in,
                neuron->synapses_in_capacity * sizeof(NeuralNetSynapse *));
  }
  neuron->synapses_in[neuron->synapses_in_count++] = synapse;
}

static void neuron_add_synapse_out(NeuralNetNeuron *neuron,
                                   NeuralNetSynapse *synapse) {
  if (neuron->synapses_out_count >= neuron->synapses_out_capacity) {
    neuron->synapses_out_capacity *= 2;
    neuron->synapses_out =
        realloc(neuron->synapses_out,
                neuron->synapses_out_capacity * sizeof(NeuralNetSynapse *));
  }
  neuron->synapses_out[neuron->synapses_out_count++] = synapse;
}

static double neuron_derivative(NeuralNetNeuron *neuron) {
  return neuron->output * (1.0 - neuron->output);
}

static void neuron_calculate_output(NeuralNetNeuron *neuron) {
  double activation = 0.0;
  for (int i = 0; i < neuron->synapses_in_count; i++) {
    NeuralNetSynapse *synapse = neuron->synapses_in[i];
    activation += synapse->weight * synapse->source_neuron->output;
  }
  activation -= neuron->threshold;
  neuron->output = 1.0 / (1.0 + exp(-activation));
}

static void neuron_output_train(NeuralNetNeuron *neuron, double target) {
  neuron->error = (target - neuron->output) * neuron_derivative(neuron);
  neuron_update_weights(neuron);
}

static void neuron_hidden_train(NeuralNetNeuron *neuron) {
  double sum = 0.0;
  for (int i = 0; i < neuron->synapses_out_count; i++) {
    NeuralNetSynapse *synapse = neuron->synapses_out[i];
    sum += synapse->prev_weight * synapse->dest_neuron->error;
  }
  neuron->error = sum * neuron_derivative(neuron);
  neuron_update_weights(neuron);
}

static void neuron_update_weights(NeuralNetNeuron *neuron) {

  for (int i = 0; i < neuron->synapses_in_count; i++) {
    NeuralNetSynapse *synapse = neuron->synapses_in[i];
    double temp_weight = synapse->weight;
    synapse->weight += (TRAIN_RATE * LEARNING_RATE * neuron->error *
                        synapse->source_neuron->output) +
                       (MOMENTUM * (synapse->weight - synapse->prev_weight));
    synapse->prev_weight = temp_weight;
  }

  double temp_threshold = neuron->threshold;
  neuron->threshold +=
      (TRAIN_RATE * LEARNING_RATE * neuron->error * -1.0) +
      (MOMENTUM * (neuron->threshold - neuron->prev_threshold));
  neuron->prev_threshold = temp_threshold;
}

static NeuralNetwork *network_new(int inputs, int hidden, int outputs) {
  NeuralNetwork *net = malloc(sizeof(NeuralNetwork));

  net->total_neurons = inputs + hidden + outputs;
  net->neurons = malloc(net->total_neurons * sizeof(NeuralNetNeuron));

  for (int i = 0; i < net->total_neurons; i++) {
    neuron_init(&net->neurons[i]);
  }

  net->input_count = inputs;
  net->hidden_count = hidden;
  net->output_count = outputs;

  net->input_layer = malloc(inputs * sizeof(int));
  net->hidden_layer = malloc(hidden * sizeof(int));
  net->output_layer = malloc(outputs * sizeof(int));

  for (int i = 0; i < inputs; i++) {
    net->input_layer[i] = i;
  }
  for (int i = 0; i < hidden; i++) {
    net->hidden_layer[i] = inputs + i;
  }
  for (int i = 0; i < outputs; i++) {
    net->output_layer[i] = inputs + hidden + i;
  }

  net->synapse_count = 0;
  net->synapse_capacity = (inputs * hidden) + (hidden * outputs);
  net->synapses = malloc(net->synapse_capacity * sizeof(NeuralNetSynapse));

  for (int i = 0; i < inputs; i++) {
    NeuralNetNeuron *source = &net->neurons[net->input_layer[i]];
    for (int j = 0; j < hidden; j++) {
      NeuralNetNeuron *dest = &net->neurons[net->hidden_layer[j]];

      NeuralNetSynapse *synapse = &net->synapses[net->synapse_count++];

      double r = Helper_next_float(1.0);
      synapse->weight = synapse->prev_weight = r * 2.0 - 1.0;

      synapse->source_neuron = source;
      synapse->dest_neuron = dest;

      neuron_add_synapse_out(source, synapse);
      neuron_add_synapse_in(dest, synapse);
    }
  }

  for (int i = 0; i < hidden; i++) {
    NeuralNetNeuron *source = &net->neurons[net->hidden_layer[i]];
    for (int j = 0; j < outputs; j++) {
      NeuralNetNeuron *dest = &net->neurons[net->output_layer[j]];

      NeuralNetSynapse *synapse = &net->synapses[net->synapse_count++];

      double r = Helper_next_float(1.0);
      synapse->weight = synapse->prev_weight = r * 2.0 - 1.0;

      synapse->source_neuron = source;
      synapse->dest_neuron = dest;

      neuron_add_synapse_out(source, synapse);
      neuron_add_synapse_in(dest, synapse);
    }
  }

  return net;
}

static void network_free(NeuralNetwork *net) {
  if (!net)
    return;

  for (int i = 0; i < net->total_neurons; i++) {
    free(net->neurons[i].synapses_in);
    free(net->neurons[i].synapses_out);
  }

  free(net->neurons);
  free(net->input_layer);
  free(net->hidden_layer);
  free(net->output_layer);
  free(net->synapses);
  free(net);
}

static void network_train(NeuralNetwork *net, double *inputs, double *targets) {

  for (int i = 0; i < net->input_count; i++) {
    net->neurons[net->input_layer[i]].output = inputs[i];
  }

  for (int i = 0; i < net->hidden_count; i++) {
    neuron_calculate_output(&net->neurons[net->hidden_layer[i]]);
  }

  for (int i = 0; i < net->output_count; i++) {
    neuron_calculate_output(&net->neurons[net->output_layer[i]]);
  }

  for (int i = 0; i < net->output_count; i++) {
    neuron_output_train(&net->neurons[net->output_layer[i]], targets[i]);
  }

  for (int i = 0; i < net->hidden_count; i++) {
    neuron_hidden_train(&net->neurons[net->hidden_layer[i]]);
  }
}

static void network_feed_forward(NeuralNetwork *net, double *inputs) {
  for (int i = 0; i < net->input_count; i++) {
    net->neurons[net->input_layer[i]].output = inputs[i];
  }

  for (int i = 0; i < net->hidden_count; i++) {
    neuron_calculate_output(&net->neurons[net->hidden_layer[i]]);
  }

  for (int i = 0; i < net->output_count; i++) {
    neuron_calculate_output(&net->neurons[net->output_layer[i]]);
  }
}

static double network_get_output(NeuralNetwork *net) {
  return net->neurons[net->output_layer[0]].output;
}

void NeuralNet_prepare(Benchmark *self) {
  NeuralNetData *data = (NeuralNetData *)self->data;

  data->result_val = 0;
  data->sum_outputs = 0.0;

  if (data->xor_net) {
    network_free(data->xor_net);
  }

  Helper_reset();

  data->xor_net = network_new(2, 10, 1);
}

void NeuralNet_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  NeuralNetData *data = (NeuralNetData *)self->data;

  double inputs_00[2] = {0, 0};
  double targets_0[1] = {0};

  double inputs_10[2] = {1, 0};
  double inputs_01[2] = {0, 1};
  double targets_1[1] = {1};

  double inputs_11[2] = {1, 1};
  double targets_0_again[1] = {0};

  for (int iter = 0; iter < 1000; iter++) {
    network_train(data->xor_net, inputs_00, targets_0);
    network_train(data->xor_net, inputs_10, targets_1);
    network_train(data->xor_net, inputs_01, targets_1);
    network_train(data->xor_net, inputs_11, targets_0_again);
  }
}

uint32_t NeuralNet_checksum(Benchmark *self) {
  NeuralNetData *data = (NeuralNetData *)self->data;

  if (!data->xor_net) {
    return 0;
  }

  double inputs_00[2] = {0, 0};
  double inputs_01[2] = {0, 1};
  double inputs_10[2] = {1, 0};
  double inputs_11[2] = {1, 1};

  double sum = 0.0;

  network_feed_forward(data->xor_net, inputs_00);
  sum += network_get_output(data->xor_net);

  network_feed_forward(data->xor_net, inputs_01);
  sum += network_get_output(data->xor_net);

  network_feed_forward(data->xor_net, inputs_10);
  sum += network_get_output(data->xor_net);

  network_feed_forward(data->xor_net, inputs_11);
  sum += network_get_output(data->xor_net);

  data->sum_outputs = sum;

  return Helper_checksum_f64(sum);
}

void NeuralNet_cleanup(Benchmark *self) {
  NeuralNetData *data = (NeuralNetData *)self->data;

  if (data->xor_net) {
    network_free(data->xor_net);
    data->xor_net = NULL;
  }
}

Benchmark *NeuralNet_create(void) {
  Benchmark *bench = Benchmark_create("Etc::NeuralNet");

  NeuralNetData *data = malloc(sizeof(NeuralNetData));
  memset(data, 0, sizeof(NeuralNetData));

  bench->data = data;

  bench->prepare = NeuralNet_prepare;
  bench->run = NeuralNet_run;
  bench->checksum = NeuralNet_checksum;
  bench->cleanup = NeuralNet_cleanup;

  return bench;
}
