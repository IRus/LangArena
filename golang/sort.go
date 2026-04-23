package main

import (
	"sort"
)

type SortBenchmark struct {
	BaseBenchmark
	data   []int
	result uint32
}

type SortQuick struct {
	BaseBenchmark
	data   []int
	result uint32
}

func (s *SortQuick) Prepare() {
	size := int(s.ConfigVal("size"))
	s.data = make([]int, size)
	for i := 0; i < size; i++ {
		s.data[i] = NextInt(1_000_000)
	}
}

func (s *SortQuick) quickSort(arr []int, low, high int) {
	if low >= high {
		return
	}

	pivot := arr[(low+high)/2]
	i, j := low, high

	for i <= j {
		for arr[i] < pivot {
			i++
		}
		for arr[j] > pivot {
			j--
		}
		if i <= j {
			arr[i], arr[j] = arr[j], arr[i]
			i++
			j--
		}
	}

	s.quickSort(arr, low, j)
	s.quickSort(arr, i, high)
}

func (s *SortQuick) Run(iteration_id int) {
	s.result += uint32(s.data[NextInt(len(s.data))])
	arr := make([]int, len(s.data))
	copy(arr, s.data)
	s.quickSort(arr, 0, len(arr)-1)
	s.result += uint32(arr[NextInt(len(arr))])
}

func (s *SortQuick) Checksum() uint32 {
	return s.result
}

type SortMerge struct {
	BaseBenchmark
	data   []int
	result uint32
}

func (s *SortMerge) Prepare() {
	size := int(s.ConfigVal("size"))
	s.data = make([]int, size)
	for i := 0; i < size; i++ {
		s.data[i] = NextInt(1_000_000)
	}
}

func (s *SortMerge) mergeSortInplace(arr []int) {
	temp := make([]int, len(arr))
	s.mergeSortHelper(arr, temp, 0, len(arr)-1)
}

func (s *SortMerge) mergeSortHelper(arr, temp []int, left, right int) {
	if left >= right {
		return
	}

	mid := (left + right) / 2
	s.mergeSortHelper(arr, temp, left, mid)
	s.mergeSortHelper(arr, temp, mid+1, right)
	s.merge(arr, temp, left, mid, right)
}

func (s *SortMerge) merge(arr, temp []int, left, mid, right int) {
	copy(temp[left:right+1], arr[left:right+1])

	i, j, k := left, mid+1, left

	for i <= mid && j <= right {
		if temp[i] <= temp[j] {
			arr[k] = temp[i]
			i++
		} else {
			arr[k] = temp[j]
			j++
		}
		k++
	}

	for i <= mid {
		arr[k] = temp[i]
		i++
		k++
	}
}

func (s *SortMerge) Run(iteration_id int) {
	s.result += uint32(s.data[NextInt(len(s.data))])
	arr := make([]int, len(s.data))
	copy(arr, s.data)
	s.mergeSortInplace(arr)
	s.result += uint32(arr[NextInt(len(arr))])
}

func (s *SortMerge) Checksum() uint32 {
	return s.result
}

type SortSelf struct {
	BaseBenchmark
	data   []int
	result uint32
}

func (s *SortSelf) Prepare() {
	size := int(s.ConfigVal("size"))
	s.data = make([]int, size)
	for i := 0; i < size; i++ {
		s.data[i] = NextInt(1_000_000)
	}
}

func (s *SortSelf) Run(iteration_id int) {
	s.result += uint32(s.data[NextInt(len(s.data))])
	arr := make([]int, len(s.data))
	copy(arr, s.data)
	sort.Ints(arr)
	s.result += uint32(arr[NextInt(len(arr))])
}

func (s *SortSelf) Checksum() uint32 {
	return s.result
}