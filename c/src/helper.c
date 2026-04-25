#include "helper.h"
#include "yyjson.h"

uint32_t Helper_last = INIT;
yyjson_doc *global_config_doc = NULL;
yyjson_mut_doc *global_config_mut = NULL;
yyjson_val *global_config_root = NULL;
bool global_config_is_array = false;
char **global_order = NULL;
yyjson_doc *global_config = NULL;
size_t global_order_count = 0;

void Helper_reset(void) { Helper_last = INIT; }

uint32_t Helper_next_int(uint32_t max) {
  Helper_last = (Helper_last * IA + IC) % IM;
  return (uint32_t)((Helper_last * (int64_t)max) / IM);
}

uint32_t Helper_next_int_range(uint32_t from, uint32_t to) {
  return Helper_next_int(to - from + 1) + from;
}

double Helper_next_float(double max) {
  Helper_last = (Helper_last * IA + IC) % IM;
  return max * Helper_last / IM;
}

uint32_t Helper_checksum_string(const char *v) {
  uint32_t hash = 5381;
  while (*v) {
    unsigned char c = (unsigned char)(*v);
    hash = ((hash << 5) + hash) + c;
    v++;
  }
  return hash;
}

uint32_t Helper_checksum_bytes(const uint8_t *data, size_t length) {
  uint32_t hash = 5381;
  for (size_t i = 0; i < length; i++) {
    hash = ((hash << 5) + hash) + data[i];
  }
  return hash;
}

uint32_t Helper_checksum_f64(double v) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.7f", v);
  return Helper_checksum_string(buffer);
}

void Helper_load_config(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Cannot open config file: %s\n", filename);
    exit(1);
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *json_data = malloc(file_size + 1);
  if (!json_data) {
    fprintf(stderr, "Memory allocation error\n");
    fclose(file);
    exit(1);
  }

  size_t read_size = fread(json_data, 1, file_size, file);
  json_data[read_size] = '\0';
  fclose(file);

  yyjson_doc *doc = yyjson_read(json_data, read_size, 0);
  free(json_data);

  if (!doc) {
    fprintf(stderr, "Error parsing JSON config\n");
    exit(1);
  }

  yyjson_val *root = yyjson_doc_get_root(doc);

  if (yyjson_is_arr(root)) {
    global_config_is_array = true;

    global_config = doc;
    global_config_root = root;

    size_t idx, max;
    yyjson_val *item;
    yyjson_arr_foreach(root, idx, max, item) {
      yyjson_val *name_val = yyjson_obj_get(item, "name");
      if (name_val && yyjson_is_str(name_val)) {
        const char *name = yyjson_get_str(name_val);

        global_order =
            realloc(global_order, sizeof(char *) * (global_order_count + 1));
        global_order[global_order_count] = strdup(name);
        global_order_count++;
      }
    }
  } else {
    global_config_is_array = false;
    global_config = doc;
    global_config_root = root;
  }
}

void Helper_free_config(void) {
  if (global_config) {
    yyjson_doc_free(global_config);
    global_config = NULL;
    global_config_root = NULL;
  }
  if (global_order) {
    for (size_t i = 0; i < global_order_count; i++) {
      free(global_order[i]);
    }
    free(global_order);
    global_order = NULL;
    global_order_count = 0;
  }
}

static yyjson_val *find_class_obj(const char *class_name) {
  if (!global_config_root)
    return NULL;

  if (global_config_is_array) {

    size_t idx, max;
    yyjson_val *item;
    yyjson_arr_foreach(global_config_root, idx, max, item) {
      yyjson_val *name_val = yyjson_obj_get(item, "name");
      if (name_val && yyjson_is_str(name_val)) {
        if (strcmp(yyjson_get_str(name_val), class_name) == 0) {
          return item;
        }
      }
    }
    return NULL;
  } else {

    return yyjson_obj_get(global_config_root, class_name);
  }
}

int64_t Helper_config_i64(const char *class_name, const char *field_name) {
  if (!global_config) {
    fprintf(stderr, "Config not loaded\n");
    return 0;
  }

  yyjson_val *class_obj = find_class_obj(class_name);
  if (!class_obj || !yyjson_is_obj(class_obj)) {
    return 0;
  }

  yyjson_val *field = yyjson_obj_get(class_obj, field_name);
  if (!field) {
    return 0;
  }

  if (yyjson_is_num(field)) {
    return (int64_t)yyjson_get_num(field);
  } else if (yyjson_is_str(field)) {
    return atoll(yyjson_get_str(field));
  } else {
    return 0;
  }
}

const char *Helper_config_s(const char *class_name, const char *field_name) {
  if (!global_config) {
    fprintf(stderr, "Config not loaded\n");
    return "";
  }

  yyjson_val *class_obj = find_class_obj(class_name);
  if (!class_obj || !yyjson_is_obj(class_obj)) {
    fprintf(stderr, "Config not found for %s\n", class_name);
    return "";
  }

  yyjson_val *field = yyjson_obj_get(class_obj, field_name);
  if (!field || !yyjson_is_str(field)) {
    return "";
  }

  return yyjson_get_str(field);
}
