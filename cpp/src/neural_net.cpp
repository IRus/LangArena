#include "neural_net.hpp"
#include <cmath>

NeuralNet::Synapse::Synapse(Neuron *source, Neuron *dest)
    : source_neuron(source), dest_neuron(dest) {
  weight = prev_weight = Helper::next_float() * 2 - 1;
}

NeuralNet::Neuron::Neuron() {
  threshold = prev_threshold = Helper::next_float() * 2 - 1;
  output = 0.0;
  error = 0.0;
}

void NeuralNet::Neuron::calculate_output() {
  double activation = 0.0;
  for (auto synapse : synapses_in) {
    activation += synapse->weight * synapse->source_neuron->output;
  }
  activation -= threshold;
  output = 1.0 / (1.0 + std::exp(-activation));
}

void NeuralNet::Neuron::output_train(double rate, double target) {
  error = (target - output) * derivative();
  update_weights(rate);
}

void NeuralNet::Neuron::hidden_train(double rate) {
  double sum = 0.0;
  for (auto synapse : synapses_out) {
    sum += synapse->prev_weight * synapse->dest_neuron->error;
  }
  error = sum * derivative();
  update_weights(rate);
}

void NeuralNet::Neuron::update_weights(double rate) {
  for (auto synapse : synapses_in) {
    double temp_weight = synapse->weight;
    synapse->weight +=
        (rate * LEARNING_RATE * error * synapse->source_neuron->output) +
        (MOMENTUM * (synapse->weight - synapse->prev_weight));
    synapse->prev_weight = temp_weight;
  }

  double temp_threshold = threshold;
  threshold += (rate * LEARNING_RATE * error * -1) +
               (MOMENTUM * (threshold - prev_threshold));
  prev_threshold = temp_threshold;
}

NeuralNet::NeuralNetwork::NeuralNetwork(int inputs, int hidden, int outputs)
    : input_layer(inputs), hidden_layer(hidden), output_layer(outputs) {

  for (auto &source : input_layer) {
    for (auto &dest : hidden_layer) {
      auto synapse = std::make_unique<Synapse>(&source, &dest);
      source.add_synapse_out(synapse.get());
      dest.add_synapse_in(synapse.get());
      synapses.push_back(std::move(synapse));
    }
  }

  for (auto &source : hidden_layer) {
    for (auto &dest : output_layer) {
      auto synapse = std::make_unique<Synapse>(&source, &dest);
      source.add_synapse_out(synapse.get());
      dest.add_synapse_in(synapse.get());
      synapses.push_back(std::move(synapse));
    }
  }
}

void NeuralNet::NeuralNetwork::train(const std::vector<double> &inputs,
                                     const std::vector<double> &targets) {
  feed_forward(inputs);

  for (size_t i = 0; i < output_layer.size(); i++) {
    output_layer[i].output_train(0.3, targets[i]);
  }

  for (auto &neuron : hidden_layer) {
    neuron.hidden_train(0.3);
  }
}

void NeuralNet::NeuralNetwork::feed_forward(const std::vector<double> &inputs) {
  for (size_t i = 0; i < input_layer.size(); i++) {
    input_layer[i].set_output(inputs[i]);
  }

  for (auto &neuron : hidden_layer) {
    neuron.calculate_output();
  }

  for (auto &neuron : output_layer) {
    neuron.calculate_output();
  }
}

std::vector<double> NeuralNet::NeuralNetwork::current_outputs() {
  std::vector<double> outputs;
  outputs.reserve(output_layer.size());
  for (const auto &neuron : output_layer) {
    outputs.push_back(neuron.get_output());
  }
  return outputs;
}

NeuralNet::NeuralNet() { xor_net = std::make_unique<NeuralNetwork>(0, 0, 0); }

std::string NeuralNet::name() const { return "Etc::NeuralNet"; }

void NeuralNet::prepare() {
  xor_net = std::make_unique<NeuralNetwork>(2, 10, 1);
}

void NeuralNet::run(int iteration_id) {
  (void)iteration_id;
  NeuralNetwork &xor_ref = *xor_net;

  static const std::vector<double> INPUT_00 = {0, 0};
  static const std::vector<double> INPUT_01 = {0, 1};
  static const std::vector<double> INPUT_10 = {1, 0};
  static const std::vector<double> INPUT_11 = {1, 1};
  static const std::vector<double> TARGET_0 = {0};
  static const std::vector<double> TARGET_1 = {1};

  for (int iter = 0; iter < 1000; iter++) {
    xor_ref.train(INPUT_00, TARGET_0);
    xor_ref.train(INPUT_10, TARGET_1);
    xor_ref.train(INPUT_01, TARGET_1);
    xor_ref.train(INPUT_11, TARGET_0);
  }
}

uint32_t NeuralNet::checksum() {
  xor_net->feed_forward({0, 0});
  auto outputs1 = xor_net->current_outputs();

  xor_net->feed_forward({0, 1});
  auto outputs2 = xor_net->current_outputs();

  xor_net->feed_forward({1, 0});
  auto outputs3 = xor_net->current_outputs();

  xor_net->feed_forward({1, 1});
  auto outputs4 = xor_net->current_outputs();

  std::vector<double> all_outputs;
  all_outputs.insert(all_outputs.end(), outputs1.begin(), outputs1.end());
  all_outputs.insert(all_outputs.end(), outputs2.begin(), outputs2.end());
  all_outputs.insert(all_outputs.end(), outputs3.begin(), outputs3.end());
  all_outputs.insert(all_outputs.end(), outputs4.begin(), outputs4.end());

  double sum = 0.0;
  for (double v : all_outputs)
    sum += v;
  return Helper::checksum_f64(sum);
}