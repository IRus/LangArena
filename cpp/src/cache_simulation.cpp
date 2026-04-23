#include "cache_simulation.hpp"
#include "helper.hpp"
#include <cstdio>

template <typename K, typename V>
CacheSimulation::LRUCache<K, V>::Node::Node(const K &k, const V &v)
    : key(k), value(v), prev(nullptr), next(nullptr) {}

template <typename K, typename V>
CacheSimulation::LRUCache<K, V>::LRUCache(int capacity)
    : capacity_(capacity), head_(nullptr), tail_(nullptr), size_(0) {}

template <typename K, typename V> CacheSimulation::LRUCache<K, V>::~LRUCache() {
  Node *current = head_;
  while (current) {
    Node *next = current->next;
    delete current;
    current = next;
  }
}

template <typename K, typename V>
void CacheSimulation::LRUCache<K, V>::move_to_front(Node *node) {
  if (node == head_)
    return;

  if (node->prev)
    node->prev->next = node->next;
  if (node->next)
    node->next->prev = node->prev;
  if (node == tail_)
    tail_ = node->prev;

  node->prev = nullptr;
  node->next = head_;
  if (head_)
    head_->prev = node;
  head_ = node;
  if (!tail_)
    tail_ = node;
}

template <typename K, typename V>
void CacheSimulation::LRUCache<K, V>::add_to_front(Node *node) {
  node->next = head_;
  if (head_)
    head_->prev = node;
  head_ = node;
  if (!tail_)
    tail_ = node;
}

template <typename K, typename V>
void CacheSimulation::LRUCache<K, V>::remove_oldest() {
  if (!tail_)
    return;

  Node *oldest = tail_;
  cache_.erase(oldest->key);

  if (oldest->prev)
    oldest->prev->next = nullptr;
  tail_ = oldest->prev;
  if (head_ == oldest)
    head_ = nullptr;

  delete oldest;
  size_--;
}

template <typename K, typename V>
std::optional<V> CacheSimulation::LRUCache<K, V>::get(const K &key) {
  auto it = cache_.find(key);
  if (it == cache_.end())
    return std::nullopt;

  Node *node = it->second;
  move_to_front(node);
  return node->value;
}

template <typename K, typename V>
void CacheSimulation::LRUCache<K, V>::put(const K &key, const V &value) {
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    Node *node = it->second;
    node->value = value;
    move_to_front(node);
    return;
  }

  if (size_ >= capacity_)
    remove_oldest();

  Node *node = new Node(key, value);
  cache_[key] = node;
  add_to_front(node);
  size_++;
}

CacheSimulation::CacheSimulation()
    : result_val(5432), values_size(config_val("values")),
      cache(config_val("size")), hits(0), misses(0) {}

std::string CacheSimulation::name() const { return "Etc::CacheSimulation"; }

void CacheSimulation::run(int iteration_id) {
  for (int i = 0; i < 1000; i++) {
    char key_buf[32];
    snprintf(key_buf, sizeof(key_buf), "item_%d",
             Helper::next_int(values_size));
    std::string key(key_buf);

    auto value = cache.get(key);
    if (value.has_value()) {
      hits++;
      char val_buf[32];
      snprintf(val_buf, sizeof(val_buf), "updated_%d", iteration_id);
      cache.put(key, std::string(val_buf));
    } else {
      misses++;
      char val_buf[32];
      snprintf(val_buf, sizeof(val_buf), "new_%d", iteration_id);
      cache.put(key, std::string(val_buf));
    }
  }
}

uint32_t CacheSimulation::checksum() {
  uint32_t result = result_val;
  result = (result << 5) + hits;
  result = (result << 5) + misses;
  result = (result << 5) + static_cast<uint32_t>(cache.size());
  return result;
}