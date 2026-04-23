package main

import (
	"bytes"
	"strings"
)

type Words struct {
	BaseBenchmark
	words       int64
	wordLen     int64
	text        string
	checksumVal uint32
}

func (w *Words) Prepare() {
	w.words = w.ConfigVal("words")
	w.wordLen = w.ConfigVal("word_len")

	chars := []byte("abcdefghijklmnopqrstuvwxyz")
	charCount := len(chars)

	var buf bytes.Buffer
	buf.Grow(int(w.words * (w.wordLen + 1)))

	for i := int64(0); i < w.words; i++ {
		length := int(NextInt(int(w.wordLen))) + NextInt(3) + 3
		for j := 0; j < length; j++ {
			idx := NextInt(charCount)
			buf.WriteByte(chars[idx])
		}
		if i < w.words-1 {
			buf.WriteByte(' ')
		}
	}

	w.text = buf.String()
}

func (w *Words) Run(iteration_id int) {

	frequencies := make(map[string]int)

	for _, word := range strings.Fields(w.text) {
		frequencies[word]++
	}

	maxWord := ""
	maxCount := 0
	for word, count := range frequencies {
		if count > maxCount {
			maxCount = count
			maxWord = word
		}
	}

	freqSize := uint32(len(frequencies))
	wordChecksum := Checksum(maxWord)

	w.checksumVal += uint32(maxCount) + wordChecksum + freqSize
}

func (w *Words) Checksum() uint32 {
	return w.checksumVal
}
