module json_bench;

import benchmark;
import helper;
import std::io;
import std::core::mem;
import std::core::string;
import std::encoding::json;
import std::collections::object;
import std::math;

struct Coord
{
    double x, y, z;
    String name;
}

struct JsonGenerateData
{
    Coord* coords;
    long n;
    String result_str;
    uint result_val;
}

fn double round_to_8_digits(double value)
{
    return math::round(value * 1e8) / 1e8;
}

fn void json_generate_prepare(Benchmark* self)
{
    JsonGenerateData* data = (JsonGenerateData*)self.data;

    mem::free(data.coords);
    data.coords = null;
    data.result_str = "";
    data.result_val = 0;

    data.coords = mem::alloc_array(Coord, (usz)data.n);

    for (long i = 0; i < data.n; i++)
    {
        data.coords[i].x = round_to_8_digits(helper::helper_next_float(1.0));
        data.coords[i].y = round_to_8_digits(helper::helper_next_float(1.0));
        data.coords[i].z = round_to_8_digits(helper::helper_next_float(1.0));

        String name = string::tformat("%.7f %d", helper::helper_next_float(1.0), helper::helper_next_int(10000));
        char* name_copy = mem::alloc_array(char, name.len + 1);
        mem::copy(name_copy, (char*)name, name.len);
        name_copy[name.len] = 0;
        data.coords[i].name = ((ZString)name_copy).str_view();
    }
}

fn void json_generate_run(Benchmark* self, int iteration_id)
{
    JsonGenerateData* data = (JsonGenerateData*)self.data;

    Object* root = object::new_obj(mem);
    defer root.free();

    Object* coordinates = object::new_obj(mem);
    defer catch coordinates.free();

    for (long i = 0; i < data.n; i++)
    {
        Object* coord = object::new_obj(mem);

        coord.set("x", data.coords[i].x);
        coord.set("y", data.coords[i].y);
        coord.set("z", data.coords[i].z);
        coord.set("name", data.coords[i].name);

        Object* opts = object::new_obj(mem);
        Object* arr = object::new_obj(mem);
        arr.push(1);
        arr.push(true);
        opts.set("1", arr);
        coord.set("opts", opts);

        coordinates.push(coord);
    }

    root.set("coordinates", coordinates);
    root.set("info", "some info");

    DString s;
    s.init(mem);
    io::fprint(&s, root)!!;
    data.result_str = s.str_view();

    if (data.result_str.starts_with("{"))
    {
        data.result_val++;
    }
}

fn uint json_generate_checksum(Benchmark* self)
{
    return ((JsonGenerateData*)self.data).result_val;
}

fn void json_generate_cleanup(Benchmark* self)
{
    JsonGenerateData* data = (JsonGenerateData*)self.data;
    if (data.coords)
    {
        for (long i = 0; i < data.n; i++)
        {
            mem::free((void*)data.coords[i].name.ptr);
        }
        mem::free(data.coords);
        data.coords = null;
    }
}

fn Benchmark* json_generate_create() @cname("JsonGenerate_create") @export
{
    Benchmark* bench = benchmark::benchmark_create("Json::Generate");

    JsonGenerateData* data = mem::new(JsonGenerateData);
    data.n = helper::helper_config_i64(bench.name, "coords");

    bench.data = data;
    bench.prepare = (PrepareFn)&json_generate_prepare;
    bench.run = (RunFn)&json_generate_run;
    bench.checksum = (ChecksumFn)&json_generate_checksum;
    bench.cleanup = (CleanupFn)&json_generate_cleanup;

    return bench;
}

fn String generate_json_for_parsing(long coords_count)
{
    Benchmark* gen_bench = json_generate_create();
    JsonGenerateData* gen_data = (JsonGenerateData*)gen_bench.data;
    gen_data.n = coords_count;
    json_generate_prepare(gen_bench);
    json_generate_run(gen_bench, 0);

    String result = gen_data.result_str;

    char* copy = mem::alloc_array(char, result.len + 1);
    mem::copy(copy, (char*)result, result.len);
    copy[result.len] = 0;

    json_generate_cleanup(gen_bench);
    mem::free(gen_bench.data);
    mem::free(gen_bench);

    return ((ZString)copy).str_view();
}

struct JsonParseDomData
{
    String json_text;
    uint result_val;
    long coords_count;
}

fn void json_parse_dom_prepare(Benchmark* self)
{
    JsonParseDomData* data = (JsonParseDomData*)self.data;
    data.coords_count = helper::helper_config_i64(self.name, "coords");
    data.json_text = generate_json_for_parsing(data.coords_count);
    data.result_val = 0;
}

fn void json_parse_dom_run(Benchmark* self, int iteration_id)
{
    JsonParseDomData* data = (JsonParseDomData*)self.data;

    Object*? doc_opt = json::parse(mem, data.json_text);
    if (catch err = doc_opt) return;

    Object* doc = doc_opt;
    defer doc.free();

    Object*? coords_opt = doc.get("coordinates");
    if (catch coords_opt) return;
    if (!coords_opt.is_array()) return;

    double x_sum = 0.0;
    double y_sum = 0.0;
    double z_sum = 0.0;
    usz len = 0;

    sz count = coords_opt.get_len();
    for (sz i = 0; i < count; i++)
    {
        Object* coord = coords_opt.get_at(i);
        if (!coord.is_map()) continue;

        if (try x_val = coord.get_float("x"))
        {
            if (try y_val = coord.get_float("y"))
            {
                if (try z_val = coord.get_float("z"))
                {
                    x_sum += x_val;
                    y_sum += y_val;
                    z_sum += z_val;
                    len++;
                }
            }
        }
    }

    if (len > 0)
    {
        double x_avg = x_sum / (double)len;
        double y_avg = y_sum / (double)len;
        double z_avg = z_sum / (double)len;

        uint checksum = helper::helper_checksum_f64(x_avg) +
                        helper::helper_checksum_f64(y_avg) +
                        helper::helper_checksum_f64(z_avg);
        data.result_val += checksum;
    }
}

fn uint json_parse_dom_checksum(Benchmark* self)
{
    return ((JsonParseDomData*)self.data).result_val;
}

fn void json_parse_dom_cleanup(Benchmark* self)
{
    JsonParseDomData* data = (JsonParseDomData*)self.data;
    data.json_text = "";
}

fn Benchmark* json_parse_dom_create() @cname("JsonParseDom_create") @export
{
    Benchmark* bench = benchmark::benchmark_create("Json::ParseDom");
    JsonParseDomData* data = mem::new(JsonParseDomData);

    bench.data = data;
    bench.prepare = (PrepareFn)&json_parse_dom_prepare;
    bench.run = (RunFn)&json_parse_dom_run;
    bench.checksum = (ChecksumFn)&json_parse_dom_checksum;
    bench.cleanup = (CleanupFn)&json_parse_dom_cleanup;

    return bench;
}

struct JsonParseMappingData
{
    String json_text;
    uint result_val;
    long coords_count;
}

fn void json_parse_mapping_prepare(Benchmark* self)
{
    JsonParseMappingData* data = (JsonParseMappingData*)self.data;
    data.coords_count = helper::helper_config_i64(self.name, "coords");
    data.json_text = generate_json_for_parsing(data.coords_count);
    data.result_val = 0;
}

fn void json_parse_mapping_run(Benchmark* self, int iteration_id)
{
    JsonParseMappingData* data = (JsonParseMappingData*)self.data;

    Object*? doc_opt = json::parse(mem, data.json_text);
    if (catch err = doc_opt) return;

    Object* doc = doc_opt;
    defer doc.free();

    Object*? coords_opt = doc.get("coordinates");
    if (catch coords_opt) return;
    if (!coords_opt.is_array()) return;

    double x_sum = 0.0;
    double y_sum = 0.0;
    double z_sum = 0.0;
    usz len = 0;

    sz count = coords_opt.get_len();
    for (sz i = 0; i < count; i++)
    {
        Object* coord = coords_opt.get_at(i);
        if (!coord.is_map()) continue;

        double xv = coord.get_float("x") ?? 0;
        double yv = coord.get_float("y") ?? 0;
        double zv = coord.get_float("z") ?? 0;

        x_sum += xv;
        y_sum += yv;
        z_sum += zv;
        len++;
    }

    if (len > 0)
    {
        double x_avg = x_sum / (double)len;
        double y_avg = y_sum / (double)len;
        double z_avg = z_sum / (double)len;

        uint checksum = helper::helper_checksum_f64(x_avg) +
                        helper::helper_checksum_f64(y_avg) +
                        helper::helper_checksum_f64(z_avg);
        data.result_val += checksum;
    }
}

fn uint json_parse_mapping_checksum(Benchmark* self)
{
    return ((JsonParseMappingData*)self.data).result_val;
}

fn void json_parse_mapping_cleanup(Benchmark* self)
{
    JsonParseMappingData* data = (JsonParseMappingData*)self.data;
    data.json_text = "";
}

fn Benchmark* json_parse_mapping_create() @cname("JsonParseMapping_create") @export
{
    Benchmark* bench = benchmark::benchmark_create("Json::ParseMapping");
    JsonParseMappingData* data = mem::new(JsonParseMappingData);

    bench.data = data;
    bench.prepare = (PrepareFn)&json_parse_mapping_prepare;
    bench.run = (RunFn)&json_parse_mapping_run;
    bench.checksum = (ChecksumFn)&json_parse_mapping_checksum;
    bench.cleanup = (CleanupFn)&json_parse_mapping_cleanup;

    return bench;
}