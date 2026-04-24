#include "sort.hpp"
#include <algorithm>

SortBenchmark::SortBenchmark() : size_val(0), result_val(0) {}

void SortBenchmark::prepare() {
  if (size_val == 0) {
    size_val = config_val("size");
    data.reserve(static_cast<size_t>(size_val));
    for (int64_t i = 0; i < size_val; i++) {
      data.push_back(Helper::next_int(1'000'000));
    }
  }
}

void SortBenchmark::run(int iteration_id) {
  (void)iteration_id;
  result_val += data[Helper::next_int(static_cast<int32_t>(size_val))];
  std::vector<int32_t> t = test();
  result_val += t[Helper::next_int(static_cast<int32_t>(size_val))];
}

uint32_t SortBenchmark::checksum() { return result_val; }

std::string SortQuick::name() const { return "Sort::Quick"; }

void SortQuick::quick_sort(std::vector<int32_t> &arr, int low, int high) {
  if (low >= high)
    return;

  int pivot = arr[(low + high) / 2];
  int i = low, j = high;

  while (i <= j) {
    while (arr[i] < pivot)
      i++;
    while (arr[j] > pivot)
      j--;
    if (i <= j) {
      std::swap(arr[i], arr[j]);
      i++;
      j--;
    }
  }

  quick_sort(arr, low, j);
  quick_sort(arr, i, high);
}

std::vector<int32_t> SortQuick::test() {
  std::vector<int32_t> arr = data;
  quick_sort(arr, 0, static_cast<int>(arr.size() - 1));
  return arr;
}

std::string SortMerge::name() const { return "Sort::Merge"; }

void SortMerge::merge_sort_inplace(std::vector<int32_t> &arr) {
  std::vector<int32_t> temp(arr.size());
  merge_sort_helper(arr, temp, 0, static_cast<int>(arr.size() - 1));
}

void SortMerge::merge_sort_helper(std::vector<int32_t> &arr,
                                  std::vector<int32_t> &temp, int left,
                                  int right) {
  if (left >= right)
    return;

  int mid = (left + right) / 2;
  merge_sort_helper(arr, temp, left, mid);
  merge_sort_helper(arr, temp, mid + 1, right);
  merge(arr, temp, left, mid, right);
}

void SortMerge::merge(std::vector<int32_t> &arr, std::vector<int32_t> &temp,
                      int left, int mid, int right) {
  for (int i = left; i <= right; i++) {
    temp[i] = arr[i];
  }

  int i = left;
  int j = mid + 1;
  int k = left;

  while (i <= mid && j <= right) {
    if (temp[i] <= temp[j]) {
      arr[k] = temp[i];
      i++;
    } else {
      arr[k] = temp[j];
      j++;
    }
    k++;
  }

  while (i <= mid) {
    arr[k] = temp[i];
    i++;
    k++;
  }
}

std::vector<int32_t> SortMerge::test() {
  std::vector<int32_t> arr = data;
  merge_sort_inplace(arr);
  return arr;
}

std::string SortSelf::name() const { return "Sort::Self"; }

std::vector<int32_t> SortSelf::test() {
  std::vector<int32_t> arr = data;
  std::sort(arr.begin(), arr.end());
  return arr;
}