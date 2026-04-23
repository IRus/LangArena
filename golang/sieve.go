package main

import (
	"math"
)

type Sieve struct {
	BaseBenchmark
	limit    int64
	checksum uint32
}

func (s *Sieve) Prepare() {
	s.limit = s.ConfigVal("limit")
	s.checksum = 0
}

func (s *Sieve) Run(iteration_id int) {
	lim := int(s.limit)
	primes := make([]byte, lim+1)
	for i := 0; i <= lim; i++ {
		primes[i] = 1
	}
	primes[0] = 0
	primes[1] = 0

	sqrtLimit := int(math.Sqrt(float64(lim)))

	for p := 2; p <= sqrtLimit; p++ {
		if primes[p] == 1 {
			for multiple := p * p; multiple <= lim; multiple += p {
				primes[multiple] = 0
			}
		}
	}

	lastPrime := 2
	count := 1

	for n := 3; n <= lim; n += 2 {
		if primes[n] == 1 {
			lastPrime = n
			count++
		}
	}

	s.checksum += uint32(lastPrime + count)
}

func (s *Sieve) Checksum() uint32 {
	return s.checksum
}
