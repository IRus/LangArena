#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class NeuralNet : public Benchmark {
private:
  class Neuron;

  class Synapse {
  public:
    double weight;
    double prev_weight;
    Neuron *source_neuron;
    Neuron *dest_neuron;

    Synapse(Neuron *source, Neuron *dest);
  };

  class Neuron {
  private:
    static constexpr double LEARNING_RATE = 1.0;
    static constexpr double MOMENTUM = 0.3;

    std::vector<Synapse *> synapses_in;
    std::vector<Synapse *> synapses_out;
    double threshold;
    double prev_threshold;
    double error;
    double output;

  public:
    Neuron();

    void calculate_output();
    double derivative() const { return output * (1 - output); }
    void output_train(double rate, double target);
    void hidden_train(double rate);
    void update_weights(double rate);
    void add_synapse_in(Synapse *synapse) { synapses_in.push_back(synapse); }
    void add_synapse_out(Synapse *synapse) { synapses_out.push_back(synapse); }
    void set_output(double val) { output = val; }
    double get_output() const { return output; }
  };

  class NeuralNetwork {
  private:
    std::vector<Neuron> input_layer;
    std::vector<Neuron> hidden_layer;
    std::vector<Neuron> output_layer;
    std::vector<std::unique_ptr<Synapse>> synapses;

  public:
    NeuralNetwork(int inputs, int hidden, int outputs);

    void train(const std::vector<double> &inputs,
               const std::vector<double> &targets);
    void feed_forward(const std::vector<double> &inputs);
    std::vector<double> current_outputs();
  };

  std::unique_ptr<NeuralNetwork> xor_net;

public:
  NeuralNet();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};