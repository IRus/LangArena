package main

import (
	"fmt"
)

type LRUCache struct {
	capacity int
	cache    map[string]*node
	head     *node
	tail     *node
	size     int
}

type node struct {
	key   string
	value string
	prev  *node
	next  *node
}

func NewLRUCache(capacity int) *LRUCache {
	return &LRUCache{
		capacity: capacity,
		cache:    make(map[string]*node),
	}
}

func (c *LRUCache) Get(key string) (string, bool) {
	if n, ok := c.cache[key]; ok {
		c.moveToFront(n)
		return n.value, true
	}
	return "", false
}

func (c *LRUCache) Put(key, value string) {
	if n, ok := c.cache[key]; ok {
		n.value = value
		c.moveToFront(n)
		return
	}

	if c.size >= c.capacity {
		c.removeOldest()
	}

	n := &node{
		key:   key,
		value: value,
	}

	c.cache[key] = n
	c.addToFront(n)
	c.size++
}

func (c *LRUCache) Size() int {
	return c.size
}

func (c *LRUCache) moveToFront(n *node) {
	if n == c.head {
		return
	}

	if n.prev != nil {
		n.prev.next = n.next
	}
	if n.next != nil {
		n.next.prev = n.prev
	}

	if n == c.tail {
		c.tail = n.prev
	}

	n.prev = nil
	n.next = c.head
	if c.head != nil {
		c.head.prev = n
	}
	c.head = n

	if c.tail == nil {
		c.tail = n
	}
}

func (c *LRUCache) addToFront(n *node) {
	n.next = c.head
	if c.head != nil {
		c.head.prev = n
	}
	c.head = n
	if c.tail == nil {
		c.tail = n
	}
}

func (c *LRUCache) removeOldest() {
	if c.tail == nil {
		return
	}

	oldest := c.tail
	delete(c.cache, oldest.key)

	if oldest.prev != nil {
		oldest.prev.next = nil
	}
	c.tail = oldest.prev

	if c.head == oldest {
		c.head = nil
	}

	c.size--
}

type CacheSimulation struct {
	BaseBenchmark
	cache  *LRUCache
	hits   int
	misses int
	result uint32
}

func (c *CacheSimulation) Prepare() {
	c.cache = NewLRUCache(int(c.ConfigVal("size")))
	c.result = 5432
}

func (c *CacheSimulation) Run(iteration_id int) {
	for k := 0; k < 1000; k += 1 {
		key := fmt.Sprintf("item_%d", NextInt(int(c.ConfigVal("values"))))
		if _, ok := c.cache.Get(key); ok {
			c.hits += 1
			c.cache.Put(key, fmt.Sprintf("updated_%d", iteration_id))
		} else {
			c.misses += 1
			c.cache.Put(key, fmt.Sprintf("new_%d", iteration_id))
		}
	}
}

func (c *CacheSimulation) Checksum() uint32 {
	c.result = (c.result << 5) + uint32(c.hits)
	c.result = (c.result << 5) + uint32(c.misses)
	c.result = (c.result << 5) + uint32(c.cache.Size())
	return c.result
}
