package main

import (
	"math"
)

type Synapse struct {
	weight       float64
	prevWeight   float64
	sourceNeuron *Neuron
	destNeuron   *Neuron
}

func NewSynapse(source, dest *Neuron) *Synapse {

	val := NextFloat(1)*2 - 1
	return &Synapse{
		weight:       val,
		prevWeight:   val,
		sourceNeuron: source,
		destNeuron:   dest,
	}
}

type Neuron struct {
	synapsesIn    []*Synapse
	synapsesOut   []*Synapse
	threshold     float64
	prevThreshold float64
	error         float64
	output        float64
}

func NewNeuron() *Neuron {

	val := NextFloat(1)*2 - 1
	return &Neuron{
		threshold:     val,
		prevThreshold: val,
		output:        0.0,
		error:         0.0,
	}
}

func (n *Neuron) CalculateOutput() {
	activation := 0.0
	for _, synapse := range n.synapsesIn {
		activation += synapse.weight * synapse.sourceNeuron.output
	}
	activation -= n.threshold
	n.output = 1.0 / (1.0 + math.Exp(-activation))
}

func (n *Neuron) Derivative() float64 {
	return n.output * (1 - n.output)
}

func (n *Neuron) OutputTrain(rate, target float64) {
	n.error = (target - n.output) * n.Derivative()
	n.UpdateWeights(rate)
}

func (n *Neuron) HiddenTrain(rate float64) {
	sum := 0.0
	for _, synapse := range n.synapsesOut {
		sum += synapse.prevWeight * synapse.destNeuron.error
	}
	n.error = sum * n.Derivative()
	n.UpdateWeights(rate)
}

func (n *Neuron) UpdateWeights(rate float64) {
	const LEARNING_RATE = 1.0
	const MOMENTUM = 0.3

	for _, synapse := range n.synapsesIn {
		tempWeight := synapse.weight
		synapse.weight += (rate * LEARNING_RATE * n.error * synapse.sourceNeuron.output) +
			(MOMENTUM * (synapse.weight - synapse.prevWeight))
		synapse.prevWeight = tempWeight
	}

	tempThreshold := n.threshold
	n.threshold += (rate * LEARNING_RATE * n.error * -1) +
		(MOMENTUM * (n.threshold - n.prevThreshold))
	n.prevThreshold = tempThreshold
}

func (n *Neuron) AddSynapseIn(synapse *Synapse) {
	n.synapsesIn = append(n.synapsesIn, synapse)
}

func (n *Neuron) AddSynapseOut(synapse *Synapse) {
	n.synapsesOut = append(n.synapsesOut, synapse)
}

func (n *Neuron) SetOutput(val float64) {
	n.output = val
}

func (n *Neuron) GetOutput() float64 {
	return n.output
}

type NeuralNetwork struct {
	inputLayer  []*Neuron
	hiddenLayer []*Neuron
	outputLayer []*Neuron
	synapses    []*Synapse
}

func NewNeuralNetwork(inputs, hidden, outputs int) *NeuralNetwork {
	nn := &NeuralNetwork{
		inputLayer:  make([]*Neuron, inputs),
		hiddenLayer: make([]*Neuron, hidden),
		outputLayer: make([]*Neuron, outputs),
	}

	for i := range nn.inputLayer {
		nn.inputLayer[i] = NewNeuron()
	}
	for i := range nn.hiddenLayer {
		nn.hiddenLayer[i] = NewNeuron()
	}
	for i := range nn.outputLayer {
		nn.outputLayer[i] = NewNeuron()
	}

	for _, source := range nn.inputLayer {
		for _, dest := range nn.hiddenLayer {
			synapse := NewSynapse(source, dest)
			source.AddSynapseOut(synapse)
			dest.AddSynapseIn(synapse)
			nn.synapses = append(nn.synapses, synapse)
		}
	}

	for _, source := range nn.hiddenLayer {
		for _, dest := range nn.outputLayer {
			synapse := NewSynapse(source, dest)
			source.AddSynapseOut(synapse)
			dest.AddSynapseIn(synapse)
			nn.synapses = append(nn.synapses, synapse)
		}
	}

	return nn
}

func (nn *NeuralNetwork) Train(inputs, targets []float64) {
	nn.FeedForward(inputs)

	for i, neuron := range nn.outputLayer {
		neuron.OutputTrain(0.3, targets[i])
	}

	for _, neuron := range nn.hiddenLayer {
		neuron.HiddenTrain(0.3)
	}
}

func (nn *NeuralNetwork) FeedForward(inputs []float64) {
	for i, neuron := range nn.inputLayer {
		neuron.SetOutput(inputs[i])
	}

	for _, neuron := range nn.hiddenLayer {
		neuron.CalculateOutput()
	}

	for _, neuron := range nn.outputLayer {
		neuron.CalculateOutput()
	}
}

func (nn *NeuralNetwork) CurrentOutputs() []float64 {
	outputs := make([]float64, len(nn.outputLayer))
	for i, neuron := range nn.outputLayer {
		outputs[i] = neuron.GetOutput()
	}
	return outputs
}

type NeuralNet struct {
	BaseBenchmark
	res    []float64
	xorNet *NeuralNetwork
}

var (
	input00 = []float64{0, 0}
	input01 = []float64{0, 1}
	input10 = []float64{1, 0}
	input11 = []float64{1, 1}
	target0 = []float64{0}
	target1 = []float64{1}
)

func (n *NeuralNet) Prepare() {
	n.xorNet = NewNeuralNetwork(2, 10, 1)
}

func (n *NeuralNet) Run(iteration_id int) {
	for i := 0; i < 1000; i++ {
		n.xorNet.Train(input00, target0)
		n.xorNet.Train(input10, target1)
		n.xorNet.Train(input01, target1)
		n.xorNet.Train(input11, target0)
	}
}

func (n *NeuralNet) Checksum() uint32 {
	n.xorNet.FeedForward(input00)
	outputs1 := n.xorNet.CurrentOutputs()

	n.xorNet.FeedForward(input01)
	outputs2 := n.xorNet.CurrentOutputs()

	n.xorNet.FeedForward(input10)
	outputs3 := n.xorNet.CurrentOutputs()

	n.xorNet.FeedForward(input11)
	outputs4 := n.xorNet.CurrentOutputs()

	allOutputs := make([]float64, 0, 4)
	allOutputs = append(allOutputs, outputs1...)
	allOutputs = append(allOutputs, outputs2...)
	allOutputs = append(allOutputs, outputs3...)
	allOutputs = append(allOutputs, outputs4...)

	sum := 0.0
	for _, v := range allOutputs {
		sum += v
	}

	return ChecksumFloat64(sum)
}