package main

import (
	"strings"
	"encoding/base64"
	"fmt"
)

type Base64Encode struct {
	BaseBenchmark
	n      int64
	bytes  []byte
	str2   string
	result uint32
}

func (b *Base64Encode) Prepare() {
	b.n = b.ConfigVal("size")
	b.bytes = []byte(strings.Repeat("a", int(b.n)))
}

func (b *Base64Encode) Run(iteration_id int) {
	b.str2 = base64.StdEncoding.EncodeToString(b.bytes)
	b.result += uint32(len(b.str2))
}

func (b *Base64Encode) Checksum() uint32 {
	resultStr := fmt.Sprintf("encode %s... to %s...: %d",
		string(b.bytes[:min(4, len(b.bytes))]),
		b.str2[:min(4, len(b.str2))],
		b.result)
	return Checksum(resultStr)
}

type Base64Decode struct {
	BaseBenchmark
	n      int64
	str2   string
	bytes  []byte
	result uint32
}

func (b *Base64Decode) Prepare() {
	b.n = b.ConfigVal("size")
	str := strings.Repeat("a", int(b.n))
	b.str2 = base64.StdEncoding.EncodeToString([]byte(str))
}

func (b *Base64Decode) Run(iteration_id int) {
	b.bytes, _ = base64.StdEncoding.DecodeString(b.str2)
	b.result += uint32(len(b.bytes))
}

func (b *Base64Decode) Checksum() uint32 {
	resultStr := fmt.Sprintf("decode %s... to %s...: %d",
		b.str2[:min(4, len(b.str2))],
		string(b.bytes[:min(4, len(b.bytes))]),
		b.result)
	return Checksum(resultStr)
}