#include "benchmark.h"

typedef struct {
  int64_t size_val;
  int32_t *data;
  uint32_t result_val;
} SortBaseData;

static void sort_base_prepare(Benchmark *self, const char *bench_name) {
  SortBaseData *data = (SortBaseData *)self->data;

  data->size_val = Helper_config_i64(bench_name, "size");
  if (data->size_val <= 0) {
    data->size_val = 100000;
  }

  data->data = malloc(data->size_val * sizeof(int32_t));

  for (int64_t i = 0; i < data->size_val; i++) {
    data->data[i] = Helper_next_int(1000000);
  }

  data->result_val = 0;
}

static void sort_base_cleanup(SortBaseData *data) {
  if (data->data) {
    free(data->data);
    data->data = NULL;
  }
}

typedef struct {
  SortBaseData base;
  int32_t *sorted_data;
} SortQuickData;

static void sort_quick_quick_sort(int32_t *arr, int64_t low, int64_t high) {
  if (low >= high)
    return;

  int32_t pivot = arr[(low + high) / 2];
  int64_t i = low, j = high;

  while (i <= j) {
    while (arr[i] < pivot)
      i++;
    while (arr[j] > pivot)
      j--;
    if (i <= j) {
      int32_t temp = arr[i];
      arr[i] = arr[j];
      arr[j] = temp;
      i++;
      j--;
    }
  }

  sort_quick_quick_sort(arr, low, j);
  sort_quick_quick_sort(arr, i, high);
}

static void sort_quick_test(SortQuickData *data) {
  if (data->sorted_data) {
    free(data->sorted_data);
  }

  data->sorted_data = malloc(data->base.size_val * sizeof(int32_t));
  memcpy(data->sorted_data, data->base.data,
         data->base.size_val * sizeof(int32_t));

  sort_quick_quick_sort(data->sorted_data, 0, data->base.size_val - 1);
}

void SortQuick_prepare(Benchmark *self) {
  SortQuickData *data = (SortQuickData *)self->data;
  sort_base_prepare(self, "Sort::Quick");
  data->sorted_data = NULL;
}

void SortQuick_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  SortQuickData *data = (SortQuickData *)self->data;

  sort_quick_test(data);

  int32_t random_index1 = Helper_next_int((uint32_t)data->base.size_val);
  int32_t random_index2 = Helper_next_int((uint32_t)data->base.size_val);

  uint32_t iteration_result = (uint32_t)data->base.data[random_index1] +
                              (uint32_t)data->sorted_data[random_index2];
  data->base.result_val += iteration_result;
}

uint32_t SortQuick_checksum(Benchmark *self) {
  SortQuickData *data = (SortQuickData *)self->data;
  return data->base.result_val;
}

void SortQuick_cleanup(Benchmark *self) {
  SortQuickData *data = (SortQuickData *)self->data;
  sort_base_cleanup(&data->base);
  if (data->sorted_data) {
    free(data->sorted_data);
    data->sorted_data = NULL;
  }
}

Benchmark *SortQuick_create(void) {
  Benchmark *bench = Benchmark_create("Sort::Quick");

  SortQuickData *data = malloc(sizeof(SortQuickData));
  memset(data, 0, sizeof(SortQuickData));

  bench->data = data;

  bench->prepare = SortQuick_prepare;
  bench->run = SortQuick_run;
  bench->checksum = SortQuick_checksum;
  bench->cleanup = SortQuick_cleanup;

  return bench;
}

typedef struct {
  SortBaseData base;
  int32_t *sorted_data;
} SortMergeData;

static void sort_merge_merge(int32_t *arr, int32_t *temp, int64_t left,
                             int64_t mid, int64_t right) {
  for (int64_t i = left; i <= right; i++) {
    temp[i] = arr[i];
  }

  int64_t i = left, j = mid + 1, k = left;

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

static void sort_merge_merge_sort_helper(int32_t *arr, int32_t *temp,
                                         int64_t left, int64_t right) {
  if (left >= right)
    return;

  int64_t mid = (left + right) / 2;
  sort_merge_merge_sort_helper(arr, temp, left, mid);
  sort_merge_merge_sort_helper(arr, temp, mid + 1, right);
  sort_merge_merge(arr, temp, left, mid, right);
}

static void sort_merge_merge_sort_inplace(int32_t *arr, int64_t size) {
  int32_t *temp = malloc(size * sizeof(int32_t));
  sort_merge_merge_sort_helper(arr, temp, 0, size - 1);
  free(temp);
}

static void sort_merge_test(SortMergeData *data) {
  if (data->sorted_data) {
    free(data->sorted_data);
  }

  data->sorted_data = malloc(data->base.size_val * sizeof(int32_t));
  memcpy(data->sorted_data, data->base.data,
         data->base.size_val * sizeof(int32_t));

  sort_merge_merge_sort_inplace(data->sorted_data, data->base.size_val);
}

void SortMerge_prepare(Benchmark *self) {
  SortMergeData *data = (SortMergeData *)self->data;
  sort_base_prepare(self, "Sort::Merge");
  data->sorted_data = NULL;
}

void SortMerge_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  SortMergeData *data = (SortMergeData *)self->data;

  sort_merge_test(data);

  int32_t random_index1 = Helper_next_int((uint32_t)data->base.size_val);
  int32_t random_index2 = Helper_next_int((uint32_t)data->base.size_val);

  uint32_t iteration_result = (uint32_t)data->base.data[random_index1] +
                              (uint32_t)data->sorted_data[random_index2];
  data->base.result_val += iteration_result;
}

uint32_t SortMerge_checksum(Benchmark *self) {
  SortMergeData *data = (SortMergeData *)self->data;
  return data->base.result_val;
}

void SortMerge_cleanup(Benchmark *self) {
  SortMergeData *data = (SortMergeData *)self->data;
  sort_base_cleanup(&data->base);
  if (data->sorted_data) {
    free(data->sorted_data);
    data->sorted_data = NULL;
  }
}

Benchmark *SortMerge_create(void) {
  Benchmark *bench = Benchmark_create("Sort::Merge");

  SortMergeData *data = malloc(sizeof(SortMergeData));
  memset(data, 0, sizeof(SortMergeData));

  bench->data = data;

  bench->prepare = SortMerge_prepare;
  bench->run = SortMerge_run;
  bench->checksum = SortMerge_checksum;
  bench->cleanup = SortMerge_cleanup;

  return bench;
}

typedef struct {
  SortBaseData base;
  int32_t *sorted_data;
} SortSelfData;

static int sort_self_compare(const void *a, const void *b) {
  int32_t ia = *(const int32_t *)a;
  int32_t ib = *(const int32_t *)b;
  return (ia > ib) - (ia < ib);
}

static void sort_self_test(SortSelfData *data) {
  if (data->sorted_data) {
    free(data->sorted_data);
  }

  data->sorted_data = malloc(data->base.size_val * sizeof(int32_t));
  memcpy(data->sorted_data, data->base.data,
         data->base.size_val * sizeof(int32_t));

  qsort(data->sorted_data, (size_t)data->base.size_val, sizeof(int32_t),
        sort_self_compare);
}

void SortSelf_prepare(Benchmark *self) {
  SortSelfData *data = (SortSelfData *)self->data;
  sort_base_prepare(self, "Sort::Self");
  data->sorted_data = NULL;
}

void SortSelf_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  SortSelfData *data = (SortSelfData *)self->data;

  sort_self_test(data);

  int32_t random_index1 = Helper_next_int((uint32_t)data->base.size_val);
  int32_t random_index2 = Helper_next_int((uint32_t)data->base.size_val);

  uint32_t iteration_result = (uint32_t)data->base.data[random_index1] +
                              (uint32_t)data->sorted_data[random_index2];
  data->base.result_val += iteration_result;
}

uint32_t SortSelf_checksum(Benchmark *self) {
  SortSelfData *data = (SortSelfData *)self->data;
  return data->base.result_val;
}

void SortSelf_cleanup(Benchmark *self) {
  SortSelfData *data = (SortSelfData *)self->data;
  sort_base_cleanup(&data->base);
  if (data->sorted_data) {
    free(data->sorted_data);
    data->sorted_data = NULL;
  }
}

Benchmark *SortSelf_create(void) {
  Benchmark *bench = Benchmark_create("Sort::Self");

  SortSelfData *data = malloc(sizeof(SortSelfData));
  memset(data, 0, sizeof(SortSelfData));

  bench->data = data;

  bench->prepare = SortSelf_prepare;
  bench->run = SortSelf_run;
  bench->checksum = SortSelf_checksum;
  bench->cleanup = SortSelf_cleanup;

  return bench;
}
