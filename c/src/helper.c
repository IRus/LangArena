#include "helper.h"
#include "cJSON.h"

uint32_t Helper_last = INIT;
cJSON *global_config = NULL;
char **global_order = NULL;
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

  cJSON *parsed = cJSON_Parse(json_data);
  free(json_data);

  if (!parsed) {
    fprintf(stderr, "Error parsing JSON config: %s\n", cJSON_GetErrorPtr());
    exit(1);
  }

  if (cJSON_IsArray(parsed)) {
    cJSON *config_map = cJSON_CreateObject();
    int array_size = cJSON_GetArraySize(parsed);
    global_order = malloc(sizeof(char *) * array_size);
    global_order_count = 0;

    cJSON *item;
    cJSON_ArrayForEach(item, parsed) {
      cJSON *name_item = cJSON_GetObjectItem(item, "name");
      if (name_item && cJSON_IsString(name_item)) {
        const char *name = name_item->valuestring;
        cJSON_AddItemToObject(config_map, name, cJSON_Duplicate(item, 1));
        global_order[global_order_count] = strdup(name);
        global_order_count++;
      }
    }

    global_config = config_map;
    cJSON_Delete(parsed);
  } else {
    global_config = parsed;
  }
}

void Helper_free_config(void) {
  if (global_config) {
    cJSON_Delete(global_config);
    global_config = NULL;
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

int64_t Helper_config_i64(const char *class_name, const char *field_name) {
  if (!global_config) {
    fprintf(stderr, "Config not loaded\n");
    return 0;
  }

  cJSON *class_obj =
      cJSON_GetObjectItemCaseSensitive(global_config, class_name);
  if (!class_obj) {
    return 0;
  }

  cJSON *field = cJSON_GetObjectItemCaseSensitive(class_obj, field_name);
  if (!field) {
    return 0;
  }

  if (cJSON_IsNumber(field)) {
    return (int64_t)field->valuedouble;
  } else if (cJSON_IsString(field)) {
    return atoll(field->valuestring);
  } else {
    return 0;
  }
}

const char *Helper_config_s(const char *class_name, const char *field_name) {
  if (!global_config) {
    fprintf(stderr, "Config not loaded\n");
    return "";
  }

  cJSON *class_obj =
      cJSON_GetObjectItemCaseSensitive(global_config, class_name);
  if (!class_obj) {
    fprintf(stderr, "Config not found for %s\n", class_name);
    return "";
  }

  cJSON *field = cJSON_GetObjectItemCaseSensitive(class_obj, field_name);
  if (!field || !cJSON_IsString(field)) {
    return "";
  }

  return field->valuestring;
}
