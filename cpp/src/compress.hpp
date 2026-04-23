#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

std::vector<uint8_t> generate_test_data(int64_t size);

class BWTEncode : public Benchmark {
public:
  struct BWTResult {
    std::vector<uint8_t> transformed;
    int32_t original_idx;

    BWTResult(std::vector<uint8_t> t, int32_t idx)
        : transformed(std::move(t)), original_idx(idx) {}
  };

private:
  BWTResult bwt_transform(const std::vector<uint8_t> &input);

public:
  int64_t size_val;
  std::vector<uint8_t> test_data;
  BWTResult bwt_result;
  uint32_t result_val;

  BWTEncode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class BWTDecode : public Benchmark {
private:
  std::vector<uint8_t> bwt_inverse(const BWTEncode::BWTResult &bwt_result);

public:
  int64_t size_val;
  std::vector<uint8_t> test_data;
  std::vector<uint8_t> inverted;
  BWTEncode::BWTResult bwt_result;
  uint32_t result_val;

  BWTDecode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class HuffEncode : public Benchmark {
public:
  class HuffmanNode {
  public:
    int32_t frequency;
    uint8_t byte_val;
    bool is_leaf;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode(int32_t freq, uint8_t byte = 0, bool leaf = true)
        : frequency(freq), byte_val(byte), is_leaf(leaf) {}
  };

  struct HuffmanCodes {
    std::vector<int32_t> code_lengths;
    std::vector<int32_t> codes;

    HuffmanCodes() : code_lengths(256, 0), codes(256, 0) {}
  };

  struct EncodedResult {
    std::vector<uint8_t> data;
    int32_t bit_count;
    std::vector<int32_t> frequencies;

    EncodedResult(std::vector<uint8_t> d, int32_t bc, std::vector<int32_t> f)
        : data(std::move(d)), bit_count(bc), frequencies(std::move(f)) {}
  };

  static std::shared_ptr<HuffmanNode>
  build_huffman_tree(const std::vector<int32_t> &frequencies);
  void build_huffman_codes(const std::shared_ptr<HuffmanNode> &node,
                           int32_t code, int32_t length, HuffmanCodes &codes);
  EncodedResult huffman_encode(const std::vector<uint8_t> &data,
                               const HuffmanCodes &codes,
                               const std::vector<int32_t> &frequencies);

public:
  int64_t size_val;
  std::vector<uint8_t> test_data;
  EncodedResult encoded;
  uint32_t result_val;

  HuffEncode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class HuffDecode : public Benchmark {
private:
  std::vector<uint8_t>
  huffman_decode(const std::vector<uint8_t> &encoded,
                 const std::shared_ptr<HuffEncode::HuffmanNode> &root,
                 int32_t bit_count);

public:
  int64_t size_val;
  std::vector<uint8_t> test_data;
  std::vector<uint8_t> decoded;
  HuffEncode::EncodedResult encoded;
  uint32_t result_val;

  HuffDecode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class ArithEncode : public Benchmark {
public:
  struct ArithEncodedResult {
    std::vector<uint8_t> data;
    int32_t bit_count;
    std::vector<int32_t> frequencies;

    ArithEncodedResult() : bit_count(0) {}
    ArithEncodedResult(std::vector<uint8_t> d, int32_t bc,
                       std::vector<int32_t> f)
        : data(std::move(d)), bit_count(bc), frequencies(std::move(f)) {}
  };

  class ArithFreqTable {
  public:
    int32_t total;
    std::vector<int32_t> low;
    std::vector<int32_t> high;

    ArithFreqTable(const std::vector<int32_t> &frequencies);
    ArithFreqTable(int32_t t, std::vector<int32_t> l, std::vector<int32_t> h)
        : total(t), low(std::move(l)), high(std::move(h)) {}
  };

  class BitOutputStream {
  private:
    uint8_t buffer = 0;
    int32_t bit_pos = 0;
    std::vector<uint8_t> bytes;
    int32_t bits_written = 0;

  public:
    void write_bit(int32_t bit);
    std::vector<uint8_t> flush();
    int32_t get_bits_written() const { return bits_written; }
    void clear();
  };

private:
  ArithEncodedResult arith_encode(const std::vector<uint8_t> &data);

public:
  int64_t size_val = 0;
  uint32_t result_val = 0;
  std::vector<uint8_t> test_data;
  ArithEncodedResult encoded;

  ArithEncode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class ArithDecode : public Benchmark {
public:
  class BitInputStream {
  private:
    const std::vector<uint8_t> &bytes;
    size_t byte_pos = 0;
    int32_t bit_pos = 0;
    uint8_t current_byte = 0;

  public:
    BitInputStream(const std::vector<uint8_t> &b);
    int32_t read_bit();
  };

private:
  std::vector<uint8_t>
  arith_decode(const ArithEncode::ArithEncodedResult &encoded);

public:
  int64_t size_val = 0;
  uint32_t result_val = 0;
  std::vector<uint8_t> test_data;
  std::vector<uint8_t> decoded;
  ArithEncode::ArithEncodedResult encoded;

  ArithDecode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class LZWEncode : public Benchmark {
public:
  struct LZWResult {
    std::vector<uint8_t> data;
    int32_t dict_size;

    LZWResult() : dict_size(256) {}
    LZWResult(std::vector<uint8_t> d, int32_t ds)
        : data(std::move(d)), dict_size(ds) {}
  };

private:
  LZWResult lzw_encode(const std::vector<uint8_t> &input);

public:
  int64_t size_val = 0;
  uint32_t result_val = 0;
  std::vector<uint8_t> test_data;
  LZWResult encoded;

  LZWEncode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};

class LZWDecode : public Benchmark {
private:
  std::vector<uint8_t> lzw_decode(const LZWEncode::LZWResult &encoded);

public:
  int64_t size_val = 0;
  uint32_t result_val = 0;
  std::vector<uint8_t> test_data;
  std::vector<uint8_t> decoded;
  LZWEncode::LZWResult encoded;

  LZWDecode();

  std::string name() const override;
  void prepare() override;
  void run(int) override;
  uint32_t checksum() override;
};