#pragma once

#include "benchmark.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

class CacheSimulation : public Benchmark {
private:
  template <typename K, typename V> class LRUCache {
  private:
    struct Node {
      K key;
      V value;
      Node *prev;
      Node *next;

      Node(const K &k, const V &v);
    };

    int capacity_;
    std::unordered_map<K, Node *> cache_;
    Node *head_;
    Node *tail_;
    int size_;

    void move_to_front(Node *node);
    void add_to_front(Node *node);
    void remove_oldest();

  public:
    LRUCache(int capacity);
    ~LRUCache();

    std::optional<V> get(const K &key);
    void put(const K &key, const V &value);
    int size() const { return size_; }
  };

  uint32_t result_val;
  int values_size;
  LRUCache<std::string, std::string> cache;
  int hits;
  int misses;

public:
  CacheSimulation();

  std::string name() const override;
  void run(int) override;
  uint32_t checksum() override;
};