module sort_bench;

import benchmark;
import helper;
import std::io;
import std::core::mem;
import std::core::string;
import std::sort;

struct SortBaseData
{
    long size_val;
    int* data;
    uint result_val;
}

fn void sort_base_prepare(Benchmark* self, String bench_name)
{
    SortBaseData* data = (SortBaseData*)self.data;
    data.size_val = helper::helper_config_i64(bench_name, "size");
    if (data.size_val <= 0) { data.size_val = 100000; }
    data.data = mem::alloc_array(int, (usz)data.size_val);
    for (long i = 0; i < data.size_val; i++)
    {
        data.data[i] = (int)helper::helper_next_int(1000000);
    }
    data.result_val = 0;
}

fn void sort_base_cleanup(SortBaseData* data)
{
    mem::free(data.data);
    data.data = null;
}

struct SortQuickData
{
    SortBaseData base;
    int* sorted_data;
}

fn void sort_quick_quick_sort(int* arr, long low, long high)
{
    if (low >= high) return;

    int pivot = arr[(low + high) / 2];
    long i = low;
    long j = high;

    while (i <= j)
    {
        while (arr[i] < pivot) { i++; }
        while (arr[j] > pivot) { j--; }
        if (i <= j)
        {
            int temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
            i++;
            j--;
        }
    }

    sort_quick_quick_sort(arr, low, j);
    sort_quick_quick_sort(arr, i, high);
}

fn void sort_quick_test(SortQuickData* data)
{
    mem::free(data.sorted_data);
    data.sorted_data = mem::alloc_array(int, (usz)data.base.size_val);
    mem::copy(data.sorted_data, data.base.data, (usz)data.base.size_val * int::size);
    sort_quick_quick_sort(data.sorted_data, 0, data.base.size_val - 1);
}

fn void sort_quick_prepare(Benchmark* self)
{
    SortQuickData* data = (SortQuickData*)self.data;
    sort_base_prepare(self, "Sort::Quick");
    data.sorted_data = null;
}

fn void sort_quick_run(Benchmark* self, int iteration_id)
{
    SortQuickData* data = (SortQuickData*)self.data;
    sort_quick_test(data);

    int random_index1 = (int)helper::helper_next_int((uint)data.base.size_val);
    int random_index2 = (int)helper::helper_next_int((uint)data.base.size_val);

    uint iteration_result = (uint)data.base.data[random_index1] +
                            (uint)data.sorted_data[random_index2];
    data.base.result_val += iteration_result;
}

fn uint sort_quick_checksum(Benchmark* self)
{
    return ((SortQuickData*)self.data).base.result_val;
}

fn void sort_quick_cleanup(Benchmark* self)
{
    SortQuickData* data = (SortQuickData*)self.data;
    sort_base_cleanup(&data.base);
    mem::free(data.sorted_data);
    data.sorted_data = null;
}

fn Benchmark* sort_quick_create() @cname("SortQuick_create") @export
{
    Benchmark* bench = benchmark::benchmark_create("Sort::Quick");
    SortQuickData* data = mem::new(SortQuickData);

    bench.data = data;
    bench.prepare = (PrepareFn)&sort_quick_prepare;
    bench.run = (RunFn)&sort_quick_run;
    bench.checksum = (ChecksumFn)&sort_quick_checksum;
    bench.cleanup = (CleanupFn)&sort_quick_cleanup;

    return bench;
}

struct SortMergeData
{
    SortBaseData base;
    int* sorted_data;
}

fn void sort_merge_merge(int* arr, int* temp, long left, long mid, long right)
{
    for (long i = left; i <= right; i++) { temp[i] = arr[i]; }

    long i = left;
    long j = mid + 1;
    long k = left;

    while (i <= mid && j <= right)
    {
        if (temp[i] <= temp[j]) { arr[k] = temp[i]; i++; }
        else { arr[k] = temp[j]; j++; }
        k++;
    }

    while (i <= mid) { arr[k] = temp[i]; i++; k++; }
}

fn void sort_merge_merge_sort_helper(int* arr, int* temp, long left, long right)
{
    if (left >= right) return;
    long mid = (left + right) / 2;
    sort_merge_merge_sort_helper(arr, temp, left, mid);
    sort_merge_merge_sort_helper(arr, temp, mid + 1, right);
    sort_merge_merge(arr, temp, left, mid, right);
}

fn void sort_merge_merge_sort_inplace(int* arr, long size)
{
    int* temp = mem::alloc_array(int, (usz)size);
    sort_merge_merge_sort_helper(arr, temp, 0, size - 1);
    mem::free(temp);
}

fn void sort_merge_test(SortMergeData* data)
{
    mem::free(data.sorted_data);
    data.sorted_data = mem::alloc_array(int, (usz)data.base.size_val);
    mem::copy(data.sorted_data, data.base.data, (usz)data.base.size_val * int::size);
    sort_merge_merge_sort_inplace(data.sorted_data, data.base.size_val);
}

fn void sort_merge_prepare(Benchmark* self)
{
    SortMergeData* data = (SortMergeData*)self.data;
    sort_base_prepare(self, "Sort::Merge");
    data.sorted_data = null;
}

fn void sort_merge_run(Benchmark* self, int iteration_id)
{
    SortMergeData* data = (SortMergeData*)self.data;
    sort_merge_test(data);

    int random_index1 = (int)helper::helper_next_int((uint)data.base.size_val);
    int random_index2 = (int)helper::helper_next_int((uint)data.base.size_val);

    uint iteration_result = (uint)data.base.data[random_index1] +
                            (uint)data.sorted_data[random_index2];
    data.base.result_val += iteration_result;
}

fn uint sort_merge_checksum(Benchmark* self)
{
    return ((SortMergeData*)self.data).base.result_val;
}

fn void sort_merge_cleanup(Benchmark* self)
{
    SortMergeData* data = (SortMergeData*)self.data;
    sort_base_cleanup(&data.base);
    mem::free(data.sorted_data);
    data.sorted_data = null;
}

fn Benchmark* sort_merge_create() @cname("SortMerge_create") @export
{
    Benchmark* bench = benchmark::benchmark_create("Sort::Merge");
    SortMergeData* data = mem::new(SortMergeData);

    bench.data = data;
    bench.prepare = (PrepareFn)&sort_merge_prepare;
    bench.run = (RunFn)&sort_merge_run;
    bench.checksum = (ChecksumFn)&sort_merge_checksum;
    bench.cleanup = (CleanupFn)&sort_merge_cleanup;

    return bench;
}

struct SortSelfData
{
    SortBaseData base;
    int* sorted_data;
}

fn int sort_self_compare(void* a, void* b)
{
    int ia = *(int*)a;
    int ib = *(int*)b;
    return (ia > ib ? 1 : 0) - (ia < ib ? 1 : 0);
}

fn void sort_self_test(SortSelfData* data)
{
    mem::free(data.sorted_data);
    data.sorted_data = mem::alloc_array(int, (usz)data.base.size_val);
    mem::copy(data.sorted_data, data.base.data, (usz)data.base.size_val * int::size);
    sort::quicksort(data.sorted_data[:data.base.size_val], &sort_self_compare);
}

fn void sort_self_prepare(Benchmark* self)
{
    SortSelfData* data = (SortSelfData*)self.data;
    sort_base_prepare(self, "Sort::Self");
    data.sorted_data = null;
}

fn void sort_self_run(Benchmark* self, int iteration_id)
{
    SortSelfData* data = (SortSelfData*)self.data;
    sort_self_test(data);

    int random_index1 = (int)helper::helper_next_int((uint)data.base.size_val);
    int random_index2 = (int)helper::helper_next_int((uint)data.base.size_val);

    uint iteration_result = (uint)data.base.data[random_index1] +
                            (uint)data.sorted_data[random_index2];
    data.base.result_val += iteration_result;
}

fn uint sort_self_checksum(Benchmark* self)
{
    return ((SortSelfData*)self.data).base.result_val;
}

fn void sort_self_cleanup(Benchmark* self)
{
    SortSelfData* data = (SortSelfData*)self.data;
    sort_base_cleanup(&data.base);
    mem::free(data.sorted_data);
    data.sorted_data = null;
}

fn Benchmark* sort_self_create() @cname("SortSelf_create") @export
{
    Benchmark* bench = benchmark::benchmark_create("Sort::Self");
    SortSelfData* data = mem::new(SortSelfData);

    bench.data = data;
    bench.prepare = (PrepareFn)&sort_self_prepare;
    bench.run = (RunFn)&sort_self_run;
    bench.checksum = (ChecksumFn)&sort_self_checksum;
    bench.cleanup = (CleanupFn)&sort_self_cleanup;

    return bench;
}