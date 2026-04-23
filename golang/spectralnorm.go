package main

import (
	"math"
)

type Spectralnorm struct {
	BaseBenchmark
	size   int64
	result uint32
	u      []float64
	v      []float64
}

func (s *Spectralnorm) Prepare() {
	s.size = s.ConfigVal("size")
	s.u = make([]float64, s.size)
	s.v = make([]float64, s.size)
	for i := range s.u {
		s.u[i] = 1.0
		s.v[i] = 1.0
	}
}

func (s *Spectralnorm) evalA(i, j int) float64 {
	return 1.0 / (float64((i+j)*(i+j+1))/2.0 + float64(i) + 1.0)
}

func (s *Spectralnorm) evalA_times_u(u []float64) []float64 {
	n := len(u)
	v := make([]float64, n)

	for i := range v {
		sum := 0.0
		for j, uj := range u {
			sum += s.evalA(i, j) * uj
		}
		v[i] = sum
	}

	return v
}

func (s *Spectralnorm) evalAt_times_u(u []float64) []float64 {
	n := len(u)
	v := make([]float64, n)

	for i := range v {
		sum := 0.0
		for j, uj := range u {
			sum += s.evalA(j, i) * uj
		}
		v[i] = sum
	}
	return v
}

func (s *Spectralnorm) evalAtA_times_u(u []float64) []float64 {
	return s.evalAt_times_u(s.evalA_times_u(u))
}

func (s *Spectralnorm) Run(iteration_id int) {
	s.v = s.evalAtA_times_u(s.u)
	s.u = s.evalAtA_times_u(s.v)
}

func (s *Spectralnorm) Checksum() uint32 {
	vBv := 0.0
	vv := 0.0
	for i := 0; i < int(s.size); i++ {
		vBv += s.u[i] * s.v[i]
		vv += s.v[i] * s.v[i]
	}
	return ChecksumFloat64(math.Sqrt(vBv / vv))
}
