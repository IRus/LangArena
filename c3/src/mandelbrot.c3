module mandelbrot;

import benchmark;
import helper;
import std::io;
import std::core::mem;
import std::core::string;

struct MandelbrotData
{
    long w;
    long h;
    char* result_bin;
    usz result_size;
    usz result_capacity;
}

fn void mandelbrot_grow_result(MandelbrotData* self, usz needed)
{
    usz new_capacity = self.result_capacity;
    while (self.result_size + needed >= new_capacity)
    {
        new_capacity = new_capacity ? new_capacity * 2 : 1024;
    }
    if (new_capacity > self.result_capacity)
    {
        self.result_bin = mem::realloc(self.result_bin, (sz)new_capacity);
        self.result_capacity = new_capacity;
    }
}

fn void mandelbrot_append(MandelbrotData* self, char[] data)
{
    mandelbrot_grow_result(self, data.len);
    mem::copy(self.result_bin + self.result_size, data.ptr, data.len);
    self.result_size += data.len;
}

fn void mandelbrot_prepare(Benchmark* self)
{
    MandelbrotData* data = (MandelbrotData*)self.data;

    data.w = helper::helper_config_i64(self.name, "w");
    data.h = helper::helper_config_i64(self.name, "h");

    if (data.w == 0) { data.w = 200; }
    if (data.h == 0) { data.h = 200; }

    data.result_size = 0;
}

fn void mandelbrot_run(Benchmark* self, int iteration_id)
{
    MandelbrotData* data = (MandelbrotData*)self.data;

    int w = (int)data.w;
    int h = (int)data.h;

    String header = string::tformat("P4\n%d %d\n", w, h);
    mandelbrot_append(data, ((char*)header)[:header.len]);

    const int ITER = 50;
    const double LIMIT = 2.0;

    int bit_num = 0;
    char byte_acc = 0;

    for (int y = 0; y < h; y++)
    {
        double ci = 2.0 * y / (double)h - 1.0;

        for (int x = 0; x < w; x++)
        {
            double cr = 2.0 * x / (double)w - 1.5;

            double zr = 0.0;
            double zi = 0.0;
            double tr = 0.0;
            double ti = 0.0;

            int i = 0;
            while (i < ITER && tr + ti <= LIMIT * LIMIT)
            {
                zi = 2.0 * zr * zi + ci;
                zr = tr - ti + cr;
                tr = zr * zr;
                ti = zi * zi;
                i++;
            }

            byte_acc <<= 1;
            if (tr + ti <= LIMIT * LIMIT) { byte_acc |= 0x01; }
            bit_num++;

            if (bit_num == 8)
            {
                mandelbrot_append(data, (&byte_acc)[:1]);
                byte_acc = 0;
                bit_num = 0;
            }
            else if (x == w - 1)
            {
                byte_acc <<= (8 - (w % 8));
                mandelbrot_append(data, (&byte_acc)[:1]);
                byte_acc = 0;
                bit_num = 0;
            }
        }
    }
}

fn uint mandelbrot_checksum(Benchmark* self)
{
    MandelbrotData* data = (MandelbrotData*)self.data;

    if (!data.result_bin || data.result_size == 0) return 0;

    return helper::helper_checksum_bytes(data.result_bin[:data.result_size]);
}

fn void mandelbrot_cleanup(Benchmark* self)
{
    MandelbrotData* data = (MandelbrotData*)self.data;

    mem::free(data.result_bin);
    data.result_bin = null;
    data.result_size = 0;
    data.result_capacity = 0;
}

fn Benchmark* mandelbrot_create() @cname("Mandelbrot_create") @export
{
    Benchmark* bench = benchmark::benchmark_create("CLBG::Mandelbrot");

    MandelbrotData* data = mem::new(MandelbrotData);

    data.w = helper::helper_config_i64("CLBG::Mandelbrot", "w");
    data.h = helper::helper_config_i64("CLBG::Mandelbrot", "h");

    if (data.w == 0) { data.w = 200; }
    if (data.h == 0) { data.h = 200; }

    bench.data = data;
    bench.prepare = (PrepareFn)&mandelbrot_prepare;
    bench.run = (RunFn)&mandelbrot_run;
    bench.checksum = (ChecksumFn)&mandelbrot_checksum;
    bench.cleanup = (CleanupFn)&mandelbrot_cleanup;

    return bench;
}