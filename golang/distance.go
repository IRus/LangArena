package main

import (
	"strings"
)

func generatePairStrings(n, m int) []struct {
	s1 string
	s2 string
} {
	pairs := make([]struct {
		s1 string
		s2 string
	}, n)
	chars := "abcdefghij"

	for i := 0; i < n; i++ {
		len1 := int(NextInt(m)) + 4
		len2 := int(NextInt(m)) + 4

		var sb1 strings.Builder
		var sb2 strings.Builder
		sb1.Grow(len1)
		sb2.Grow(len2)

		for j := 0; j < len1; j++ {
			sb1.WriteByte(chars[NextInt(10)])
		}
		for j := 0; j < len2; j++ {
			sb2.WriteByte(chars[NextInt(10)])
		}

		pairs[i] = struct {
			s1 string
			s2 string
		}{sb1.String(), sb2.String()}
	}

	return pairs
}

type Jaro struct {
	BaseBenchmark
	count int
	size  int
	pairs []struct {
		s1 string
		s2 string
	}
	result uint32
}

func (j *Jaro) Prepare() {
	j.count = int(j.ConfigVal("count"))
	j.size = int(j.ConfigVal("size"))
	j.pairs = generatePairStrings(j.count, j.size)
	j.result = 0
}

func (j *Jaro) jaro(s1, s2 string) float64 {

	bytes1 := []byte(s1)
	bytes2 := []byte(s2)

	len1 := len(bytes1)
	len2 := len(bytes2)

	if len1 == 0 || len2 == 0 {
		return 0.0
	}

	matchDist := max(len1, len2)/2 - 1
	if matchDist < 0 {
		matchDist = 0
	}

	s1Matches := make([]bool, len1)
	s2Matches := make([]bool, len2)

	matches := 0
	for i := 0; i < len1; i++ {
		start := max(0, i-matchDist)
		end := min(len2-1, i+matchDist)

		for j := start; j <= end; j++ {
			if !s2Matches[j] && bytes1[i] == bytes2[j] {
				s1Matches[i] = true
				s2Matches[j] = true
				matches++
				break
			}
		}
	}

	if matches == 0 {
		return 0.0
	}

	transpositions := 0
	k := 0
	for i := 0; i < len1; i++ {
		if s1Matches[i] {
			for k < len2 && !s2Matches[k] {
				k++
			}
			if k < len2 {
				if bytes1[i] != bytes2[k] {
					transpositions++
				}
				k++
			}
		}
	}
	transpositions /= 2

	m := float64(matches)
	return (m/float64(len1) + m/float64(len2) + (m-float64(transpositions))/m) / 3.0
}

func (j *Jaro) Run(iterationID int) {
	for _, pair := range j.pairs {
		j.result += uint32(j.jaro(pair.s1, pair.s2) * 1000)
	}
}

func (j *Jaro) Checksum() uint32 {
	return j.result
}

func (j *Jaro) Name() string {
	return "Distance::Jaro"
}

type NGram struct {
	BaseBenchmark
	count int
	size  int
	pairs []struct {
		s1 string
		s2 string
	}
	result uint32
}

const nGramN = 4

func (n *NGram) Prepare() {
	n.count = int(n.ConfigVal("count"))
	n.size = int(n.ConfigVal("size"))
	n.pairs = generatePairStrings(n.count, n.size)
	n.result = 0
}

func (n *NGram) ngram(s1, s2 string) float64 {
	if len(s1) < nGramN || len(s2) < nGramN {
		return 0.0
	}

	bytes1 := []byte(s1)
	bytes2 := []byte(s2)

	grams1 := make(map[uint32]int, len(bytes1))

	for i := 0; i <= len(bytes1)-nGramN; i++ {
		gram := (uint32(bytes1[i]) << 24) |
			(uint32(bytes1[i+1]) << 16) |
			(uint32(bytes1[i+2]) << 8) |
			uint32(bytes1[i+3])

		grams1[gram]++
	}

	grams2 := make(map[uint32]int, len(bytes2))
	intersection := 0

	for i := 0; i <= len(bytes2)-nGramN; i++ {
		gram := (uint32(bytes2[i]) << 24) |
			(uint32(bytes2[i+1]) << 16) |
			(uint32(bytes2[i+2]) << 8) |
			uint32(bytes2[i+3])

		grams2[gram]++

		if cnt1, ok := grams1[gram]; ok && grams2[gram] <= cnt1 {
			intersection++
		}
	}

	total := len(grams1) + len(grams2)
	if total > 0 {
		return float64(intersection) / float64(total)
	}
	return 0.0
}

func (n *NGram) Run(iterationID int) {
	for _, pair := range n.pairs {
		n.result += uint32(n.ngram(pair.s1, pair.s2) * 1000)
	}
}

func (n *NGram) Checksum() uint32 {
	return n.result
}

func (n *NGram) Name() string {
	return "Distance::NGram"
}
