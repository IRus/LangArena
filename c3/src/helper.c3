module helper;

import std::io;
import std::io::file;
import std::core::mem;
import std::core::string;
import std::encoding::json;
import std::collections::object;
import std::os;

const IM = 139968;
const IA = 3877;
const IC = 29573;
const INIT = 42;

uint helper_last = INIT;

char** global_order;
usz global_order_count;
usz global_order_capacity;

fn void global_order_push(char* str)
{
    if (global_order_count >= global_order_capacity)
    {
        global_order_capacity = global_order_capacity ? global_order_capacity * 2 : 16;
        global_order = mem::realloc(global_order, global_order_capacity * usz.sizeof);
    }
    global_order[global_order_count] = str;
    global_order_count++;
}

fn void helper_reset()
{
    helper_last = INIT;
}

fn uint helper_next_int(uint max)
{
    helper_last = (helper_last * IA + IC) % IM;
    return (uint)((helper_last * (long)max) / IM);
}

fn uint helper_next_int_range(uint from, uint to)
{
    return helper_next_int(to - from + 1) + from;
}

fn double helper_next_float(double max)
{
    helper_last = (helper_last * IA + IC) % IM;
    return max * helper_last / IM;
}

fn uint helper_checksum_string(String v)
{
    uint hash = 5381;
    foreach (c : v)
    {
        hash = ((hash << 5) + hash) + (uint)c;
    }
    return hash;
}

fn uint helper_checksum_bytes(char[] data)
{
    uint hash = 5381;
    foreach (b : data)
    {
        hash = ((hash << 5) + hash) + (uint)b;
    }
    return hash;
}

fn uint helper_checksum_f64(double v)
{
    String s = string::tformat("%.7f", v);
    return helper_checksum_string(s);
}

Object* global_config;
bool global_config_is_array;

fn void? helper_load_config(String filename)
{
    File fh = file::open(filename, "rb")!;
    defer (void)fh.close();

    usz len = fh.seek(0, END)!;
    fh.seek(0, SET)!;

    char* json_data = mem::alloc_array(char, len + 1);
    usz read = fh.read(json_data[:len])!;
    json_data[read] = 0;

    Object*? doc_opt = json::parse_string(mem, (String)json_data[:read]);
    mem::free(json_data);

    if (catch err = doc_opt)
    {
        io::eprintfn("Error parsing JSON config");
        os::exit(1);
    }

    global_config = doc_opt;

    if (global_config.is_array())
    {
        global_config_is_array = true;

        usz count = global_config.get_len();
        for (usz i = 0; i < count; i++)
        {
            Object* item = global_config.get_at(i);
            if (try name_val = item.get("name"))
            {
                if (name_val.is_string())
                {
                    String name = name_val.s;
                    char* c_name = mem::alloc_array(char, name.len + 1);
                    mem::copy(c_name, (char*)name, name.len);
                    c_name[name.len] = 0;
                    global_order_push(c_name);
                }
            }
        }
    }
    else
    {
        global_config_is_array = false;
    }

    return;
}

fn void helper_free_config()
{
    if (global_config)
    {
        global_config.free();
        global_config = null;
    }
    if (global_order)
    {
        for (usz i = 0; i < global_order_count; i++)
        {
            mem::free(global_order[i]);
        }
        mem::free(global_order);
        global_order = null;
        global_order_count = 0;
        global_order_capacity = 0;
    }
}

fn Object* find_class_obj(String class_name)
{
    if (!global_config) return null;

    if (global_config_is_array)
    {
        usz count = global_config.get_len();
        for (usz i = 0; i < count; i++)
        {
            Object* item = global_config.get_at(i);
            if (try name_val = item.get("name"))
            {
                if (name_val.is_string() && name_val.s == class_name)
                {
                    return item;
                }
            }
        }
        return null;
    }
    else
    {
        if (try val = global_config.get(class_name))
        {
            return val;
        }
        return null;
    }
}

fn long helper_config_i64(String class_name, String field_name)
{
    if (!global_config)
    {
        io::eprintfn("Config not loaded");
        return 0;
    }

    Object* class_obj = find_class_obj(class_name);
    if (!class_obj || !class_obj.is_map()) return 0;

    Object*? field_opt = class_obj.get(field_name);
    if (catch field_opt) return 0;

    if (field_opt.is_float())
    {
        return (long)field_opt.f;
    }
    else if (field_opt.is_int())
    {
        return (long)field_opt.i;
    }
    else if (field_opt.is_string())
    {
        return (long)(field_opt.s.to_long() ?? 0);
    }
    return 0;
}

fn String helper_config_s(String class_name, String field_name)
{
    if (!global_config)
    {
        io::eprintfn("Config not loaded");
        return "";
    }

    Object* class_obj = find_class_obj(class_name);
    if (!class_obj || !class_obj.is_map())
    {
        return "";
    }

    Object*? field_opt = class_obj.get(field_name);
    if (catch field_opt) return "";

    if (!field_opt.is_string())
    {
        return "";
    }

    return field_opt.s;
}