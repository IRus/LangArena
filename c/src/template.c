#include "benchmark.h"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

static const char *FIRST_NAMES[] = {"John",    "Jane",  "Bob",   "Alice",
                                    "Charlie", "Diana", "Sarah", "Mike"};
static const char *LAST_NAMES[] = {"Smith",  "Johnson", "Brown",  "Taylor",
                                   "Wilson", "Davis",   "Miller", "Jones"};
static const char *CITIES[] = {"New York", "Los Angeles", "Chicago",
                               "Houston",  "Phoenix",     "San Francisco"};
static const char *LOREM =
    "Lorem {ipsum} dolor {sit} amet, consectetur adipiscing elit. Sed do "
    "eiusmod tempor incididunt ut labore {et} dolore magna aliqua. ";

#define FIRST_NAMES_COUNT 8
#define LAST_NAMES_COUNT 8
#define CITIES_COUNT 6

typedef struct VarEntry {
  char *key;
  char *value;
  UT_hash_handle hh;
} VarEntry;

typedef struct {
  int count;
  uint32_t checksum_val;
  char *text;
  size_t text_size;
  char *rendered;
  size_t rendered_size;
  VarEntry *vars;
} TemplateBaseData;

typedef struct {
  TemplateBaseData base;
  pcre2_code *re;
  pcre2_match_data *match_data;
} TemplateRegexData;

typedef struct {
  TemplateBaseData base;
} TemplateParseData;

static const char *TEMPLATE_REGEX_PATTERN = "{{(.*?)}}";

static void add_var(TemplateBaseData *data, const char *key,
                    const char *value) {
  VarEntry *entry = malloc(sizeof(VarEntry));
  entry->key = strdup(key);
  entry->value = strdup(value);
  HASH_ADD_KEYPTR(hh, data->vars, entry->key, strlen(entry->key), entry);
}

static const char *get_var(TemplateBaseData *data, const char *key) {
  VarEntry *entry = NULL;
  HASH_FIND_STR(data->vars, key, entry);
  return entry ? entry->value : NULL;
}

static void clear_vars(TemplateBaseData *data) {
  VarEntry *current, *tmp;
  HASH_ITER(hh, data->vars, current, tmp) {
    HASH_DEL(data->vars, current);
    free(current->key);
    free(current->value);
    free(current);
  }
}

static int prepare_template(TemplateBaseData *data) {
  if (data->text) {
    free(data->text);
    data->text = NULL;
  }

  clear_vars(data);

  size_t estimated_size = data->count * 200 + 1;
  char *text_buf = malloc(estimated_size);
  if (!text_buf)
    return -1;

  size_t pos = 0;

  pos += sprintf(text_buf + pos, "<html><body>");
  pos += sprintf(text_buf + pos, "<h1>{{TITLE}}</h1>");
  add_var(data, "TITLE", "Template title");
  pos += sprintf(text_buf + pos, "<p>");
  pos += sprintf(text_buf + pos, "%s", LOREM);
  pos += sprintf(text_buf + pos, "</p>");
  pos += sprintf(text_buf + pos, "<table>");

  for (int i = 0; i < data->count; i++) {
    if (i % 3 == 0) {
      pos += sprintf(text_buf + pos, "<!-- {comment} -->");
    }
    pos += sprintf(text_buf + pos, "<tr>");

    char key_buf[32];

    pos += sprintf(text_buf + pos, "<td>{{ FIRST_NAME%d }}</td>", i);
    sprintf(key_buf, "FIRST_NAME%d", i);
    add_var(data, key_buf, FIRST_NAMES[i % FIRST_NAMES_COUNT]);

    pos += sprintf(text_buf + pos, "<td>{{LAST_NAME%d}}</td>", i);
    sprintf(key_buf, "LAST_NAME%d", i);
    add_var(data, key_buf, LAST_NAMES[i % LAST_NAMES_COUNT]);

    pos += sprintf(text_buf + pos, "<td>{{  CITY%d  }}</td>", i);
    sprintf(key_buf, "CITY%d", i);
    add_var(data, key_buf, CITIES[i % CITIES_COUNT]);

    pos += sprintf(text_buf + pos, "<td>{balance: %d}</td>", i % 100);
    pos += sprintf(text_buf + pos, "</tr>\n");
  }

  pos += sprintf(text_buf + pos, "</table>");
  pos += sprintf(text_buf + pos, "</body></html>");

  data->text = text_buf;
  data->text_size = pos;

  return 0;
}

void TemplateRegex_prepare(Benchmark *self) {
  TemplateRegexData *data = (TemplateRegexData *)self->data;

  data->base.count = (int)Helper_config_i64(self->name, "count");
  prepare_template(&data->base);

  if (!data->re) {
    int errornumber;
    PCRE2_SIZE erroroffset;

    data->re = pcre2_compile(
        (PCRE2_SPTR)TEMPLATE_REGEX_PATTERN, PCRE2_ZERO_TERMINATED,
        PCRE2_UTF | PCRE2_NO_UTF_CHECK, &errornumber, &erroroffset, NULL);

    if (data->re) {
      pcre2_jit_compile(data->re, PCRE2_JIT_COMPLETE);
      data->match_data = pcre2_match_data_create_from_pattern(data->re, NULL);
    }
  }
}

void TemplateRegex_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  TemplateRegexData *data = (TemplateRegexData *)self->data;
  TemplateBaseData *base = &data->base;

  if (!data->re || !data->match_data || !base->text)
    return;

  size_t estimated_size = base->text_size * 2;
  char *result_buf = malloc(estimated_size);
  if (!result_buf)
    return;

  size_t result_pos = 0;
  PCRE2_SIZE start_offset = 0;
  PCRE2_SPTR subject = (PCRE2_SPTR)base->text;
  PCRE2_SIZE subject_len = base->text_size;

  size_t last_pos = 0;

  while (1) {
    int rc = pcre2_jit_match(data->re, subject, subject_len, start_offset, 0,
                             data->match_data, NULL);

    if (rc < 0)
      break;

    PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(data->match_data);
    PCRE2_SIZE match_start = ovector[0];
    PCRE2_SIZE match_end = ovector[1];

    if (match_start > last_pos) {
      size_t len = match_start - last_pos;
      memcpy(result_buf + result_pos, base->text + last_pos, len);
      result_pos += len;
    }

    PCRE2_SIZE key_start = ovector[2];
    PCRE2_SIZE key_end = ovector[3];

    if (key_end > key_start) {
      size_t key_len = key_end - key_start;
      char *key = malloc(key_len + 1);
      memcpy(key, base->text + key_start, key_len);
      key[key_len] = '\0';

      char *start = key;
      while (*start == ' ' || *start == '\t')
        start++;
      char *end = key + key_len - 1;
      while (end > start && (*end == ' ' || *end == '\t'))
        end--;
      *(end + 1) = '\0';

      const char *value = get_var(base, start);
      if (value) {
        size_t value_len = strlen(value);
        memcpy(result_buf + result_pos, value, value_len);
        result_pos += value_len;
      }

      free(key);
    }

    last_pos = match_end;
    start_offset = match_end;

    if (start_offset >= subject_len)
      break;
  }

  if (last_pos < base->text_size) {
    size_t len = base->text_size - last_pos;
    memcpy(result_buf + result_pos, base->text + last_pos, len);
    result_pos += len;
  }

  result_buf[result_pos] = '\0';

  if (base->rendered) {
    free(base->rendered);
  }
  base->rendered = result_buf;
  base->rendered_size = result_pos;
  base->checksum_val += (uint32_t)result_pos;
}

uint32_t TemplateRegex_checksum(Benchmark *self) {
  TemplateRegexData *data = (TemplateRegexData *)self->data;
  TemplateBaseData *base = &data->base;
  return base->checksum_val + Helper_checksum_string(base->rendered);
}

void TemplateRegex_cleanup(Benchmark *self) {
  TemplateRegexData *data = (TemplateRegexData *)self->data;
  TemplateBaseData *base = &data->base;

  if (base->text) {
    free(base->text);
    base->text = NULL;
  }

  if (base->rendered) {
    free(base->rendered);
    base->rendered = NULL;
  }

  clear_vars(base);

  if (data->match_data) {
    pcre2_match_data_free(data->match_data);
    data->match_data = NULL;
  }

  if (data->re) {
    pcre2_code_free(data->re);
    data->re = NULL;
  }

  base->text_size = 0;
  base->rendered_size = 0;
  base->checksum_val = 0;
}

Benchmark *TemplateRegex_create(void) {
  Benchmark *bench = Benchmark_create("Template::Regex");

  TemplateRegexData *data = calloc(1, sizeof(TemplateRegexData));

  bench->data = data;
  bench->prepare = TemplateRegex_prepare;
  bench->run = TemplateRegex_run;
  bench->checksum = TemplateRegex_checksum;
  bench->cleanup = TemplateRegex_cleanup;

  return bench;
}

void TemplateParse_prepare(Benchmark *self) {
  TemplateParseData *data = (TemplateParseData *)self->data;

  data->base.count = (int)Helper_config_i64(self->name, "count");
  prepare_template(&data->base);
}

void TemplateParse_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  TemplateParseData *data = (TemplateParseData *)self->data;
  TemplateBaseData *base = &data->base;

  if (!base->text)
    return;

  size_t estimated_size = (size_t)(base->text_size * 1.5) + 1;
  char *result_buf = malloc(estimated_size);
  if (!result_buf)
    return;

  size_t result_pos = 0;
  size_t i = 0;
  size_t len = base->text_size;

  while (i < len) {
    if (i + 1 < len && base->text[i] == '{' && base->text[i + 1] == '{') {
      size_t j = i + 2;
      while (j + 1 < len) {
        if (base->text[j] == '}' && base->text[j + 1] == '}') {
          break;
        }
        j++;
      }

      if (j + 1 < len) {
        size_t key_len = j - i - 2;
        char *key = malloc(key_len + 1);
        memcpy(key, base->text + i + 2, key_len);
        key[key_len] = '\0';

        char *start = key;
        while (*start == ' ' || *start == '\t')
          start++;
        char *end = key + key_len - 1;
        while (end > start && (*end == ' ' || *end == '\t'))
          end--;
        *(end + 1) = '\0';

        const char *value = get_var(base, start);
        if (value) {
          size_t value_len = strlen(value);
          memcpy(result_buf + result_pos, value, value_len);
          result_pos += value_len;
        }

        free(key);
        i = j + 2;
        continue;
      }
    }

    result_buf[result_pos++] = base->text[i];
    i++;
  }

  result_buf[result_pos] = '\0';

  if (base->rendered) {
    free(base->rendered);
  }
  base->rendered = result_buf;
  base->rendered_size = result_pos;
  base->checksum_val += (uint32_t)result_pos;
}

uint32_t TemplateParse_checksum(Benchmark *self) {
  TemplateParseData *data = (TemplateParseData *)self->data;
  TemplateBaseData *base = &data->base;
  return base->checksum_val + Helper_checksum_string(base->rendered);
}

void TemplateParse_cleanup(Benchmark *self) {
  TemplateParseData *data = (TemplateParseData *)self->data;
  TemplateBaseData *base = &data->base;

  if (base->text) {
    free(base->text);
    base->text = NULL;
  }

  if (base->rendered) {
    free(base->rendered);
    base->rendered = NULL;
  }

  clear_vars(base);

  base->text_size = 0;
  base->rendered_size = 0;
  base->checksum_val = 0;
}

Benchmark *TemplateParse_create(void) {
  Benchmark *bench = Benchmark_create("Template::Parse");

  TemplateParseData *data = calloc(1, sizeof(TemplateParseData));

  bench->data = data;
  bench->prepare = TemplateParse_prepare;
  bench->run = TemplateParse_run;
  bench->checksum = TemplateParse_checksum;
  bench->cleanup = TemplateParse_cleanup;

  return bench;
}
