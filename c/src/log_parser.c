#include "benchmark.h"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

typedef struct {
  int lines_count;
  char *log;
  size_t log_size;
  uint32_t checksum_val;

  pcre2_code *compiled_patterns[13];
  pcre2_match_data *match_data[13];
} LogParserData;

static const char *PATTERN_NAMES[] = {
    "errors",        "bots",          "suspicious", "ips",    "api_calls",
    "post_requests", "auth_attempts", "methods",    "emails", "passwords",
    "tokens",        "sessions",      "peak_hours"};

static const char *PATTERNS[] = {
    " [5][0-9]{2} | [4][0-9]{2} ",
    "(?i)bot|crawler|scanner|spider|indexing|crawl|robot|spider",
    "(?i)etc/passwd|wp-admin|\\.\\./",
    "\\d+\\.\\d+\\.\\d+\\.35",
    "/api/[^ \" ]+",
    "POST [^ ]* HTTP",
    "(?i)/login|/signin",
    "(?i)get|post|put",
    "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}",
    "password=[^&\\s\"]+",
    "token=[^&\\s\"]+|api[_-]?key=[^&\\s\"]+",
    "session[_-]?id=[^&\\s\"]+",
    "\\[\\d+/\\w+/\\d+:1[3-7]:\\d+:\\d+ [+\\-]\\d+\\]"};

#define PATTERNS_COUNT 13

static const char *IPS[255];
static const char *METHODS[] = {"GET", "POST", "PUT", "DELETE"};
static const char *PATHS[] = {"/index.html", "/api/users",
                              "/admin",      "/images/logo.png",
                              "/etc/passwd", "/wp-admin/setup.php"};
static int STATUSES[] = {200, 201, 301, 302, 400, 401, 403, 404, 500, 502, 503};
static const char *AGENTS[] = {"Mozilla/5.0", "Googlebot/2.1", "curl/7.68.0",
                               "scanner/2.0"};
static const char *USERS[] = {"john", "jane", "alex",  "sarah",
                              "mike", "anna", "david", "elena"};
static const char *DOMAINS[] = {"example.com", "gmail.com",   "yahoo.com",
                                "hotmail.com", "company.org", "mail.ru"};

static void init_ips(void) {
  static int initialized = 0;
  if (initialized)
    return;

  for (int i = 0; i < 255; i++) {
    char *ip = malloc(16);
    snprintf(ip, 16, "192.168.1.%d", i + 1);
    IPS[i] = ip;
  }
  initialized = 1;
}

static void generate_log_line(char *buffer, size_t *pos, int i) {
  if (i % 3 == 0) {
    *pos += sprintf(buffer + *pos,
                    "%s - - [%d/Oct/2023:%d:55:36 +0000] \"%s "
                    "/login?email=%s%d@%s&password=secret%d HTTP/1.1\" %d 2326 "
                    "\"http://%s\" \"%s\"\n",
                    IPS[i % 255], i % 31, i % 60, METHODS[i % 4], USERS[i % 8],
                    i % 100, DOMAINS[i % 6], i % 10000, STATUSES[i % 11],
                    DOMAINS[i % 6], AGENTS[i % 4]);
  } else if (i % 5 == 0) {
    char token[200] = "";
    for (int j = 0; j < (i % 3) + 1; j++) {
      strcat(token, "abcdef123456");
    }
    *pos +=
        sprintf(buffer + *pos,
                "%s - - [%d/Oct/2023:%d:55:36 +0000] \"%s /api/data?token=%s "
                "HTTP/1.1\" %d 2326 \"http://%s\" \"%s\"\n",
                IPS[i % 255], i % 31, i % 60, METHODS[i % 4], token,
                STATUSES[i % 11], DOMAINS[i % 6], AGENTS[i % 4]);
  } else if (i % 7 == 0) {
    *pos += sprintf(buffer + *pos,
                    "%s - - [%d/Oct/2023:%d:55:36 +0000] \"%s "
                    "/user/profile?session_id=sess_%x HTTP/1.1\" %d 2326 "
                    "\"http://%s\" \"%s\"\n",
                    IPS[i % 255], i % 31, i % 60, METHODS[i % 4], i * 12345,
                    STATUSES[i % 11], DOMAINS[i % 6], AGENTS[i % 4]);
  } else {
    *pos += sprintf(buffer + *pos,
                    "%s - - [%d/Oct/2023:%d:55:36 +0000] \"%s %s HTTP/1.1\" %d "
                    "2326 \"http://%s\" \"%s\"\n",
                    IPS[i % 255], i % 31, i % 60, METHODS[i % 4], PATHS[i % 6],
                    STATUSES[i % 11], DOMAINS[i % 6], AGENTS[i % 4]);
  }
}

void LogParser_prepare(Benchmark *self) {
  LogParserData *data = (LogParserData *)self->data;

  init_ips();

  if (data->log) {
    free(data->log);
    data->log = NULL;
  }

  data->lines_count = (int)Helper_config_i64(self->name, "lines_count");

  size_t estimated_size = data->lines_count * 200 + 1;
  char *log_buf = malloc(estimated_size);
  if (!log_buf)
    return;

  size_t pos = 0;
  for (int i = 0; i < data->lines_count; i++) {
    generate_log_line(log_buf, &pos, i);
  }
  log_buf[pos] = '\0';

  data->log = log_buf;
  data->log_size = pos;

  for (int i = 0; i < PATTERNS_COUNT; i++) {
    int errornumber;
    PCRE2_SIZE erroroffset;

    data->compiled_patterns[i] = pcre2_compile(
        (PCRE2_SPTR)PATTERNS[i], PCRE2_ZERO_TERMINATED,
        PCRE2_UTF | PCRE2_NO_UTF_CHECK, &errornumber, &erroroffset, NULL);

    if (data->compiled_patterns[i]) {
      pcre2_jit_compile(data->compiled_patterns[i], PCRE2_JIT_COMPLETE);
      data->match_data[i] = pcre2_match_data_create_from_pattern(
          data->compiled_patterns[i], NULL);
    }
  }
}

void LogParser_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  LogParserData *data = (LogParserData *)self->data;

  int matches[PATTERNS_COUNT] = {0};
  (void)PATTERN_NAMES;

  for (int i = 0; i < PATTERNS_COUNT; i++) {
    if (!data->compiled_patterns[i])
      continue;

    PCRE2_SIZE start_offset = 0;
    PCRE2_SPTR subject = (PCRE2_SPTR)data->log;
    PCRE2_SIZE subject_len = data->log_size;

    while (1) {
      int rc = pcre2_jit_match(data->compiled_patterns[i], subject, subject_len,
                               start_offset, 0, data->match_data[i], NULL);

      if (rc < 0)
        break;

      matches[i]++;

      PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(data->match_data[i]);
      start_offset = ovector[1];

      if (start_offset >= subject_len)
        break;
    }
  }

  uint32_t total = 0;
  for (int i = 0; i < PATTERNS_COUNT; i++) {
    total += matches[i];
  }
  data->checksum_val += total;
}

uint32_t LogParser_checksum(Benchmark *self) {
  LogParserData *data = (LogParserData *)self->data;
  return data->checksum_val;
}

void LogParser_cleanup(Benchmark *self) {
  LogParserData *data = (LogParserData *)self->data;

  if (data->log) {
    free(data->log);
    data->log = NULL;
  }

  for (int i = 0; i < PATTERNS_COUNT; i++) {
    if (data->compiled_patterns[i]) {
      pcre2_code_free(data->compiled_patterns[i]);
      data->compiled_patterns[i] = NULL;
    }
    if (data->match_data[i]) {
      pcre2_match_data_free(data->match_data[i]);
      data->match_data[i] = NULL;
    }
  }

  data->log_size = 0;
  data->checksum_val = 0;
}

Benchmark *LogParser_create(void) {
  Benchmark *bench = Benchmark_create("Etc::LogParser");

  LogParserData *data = calloc(1, sizeof(LogParserData));

  for (int i = 0; i < PATTERNS_COUNT; i++) {
    data->compiled_patterns[i] = NULL;
    data->match_data[i] = NULL;
  }

  bench->data = data;
  bench->prepare = LogParser_prepare;
  bench->run = LogParser_run;
  bench->checksum = LogParser_checksum;
  bench->cleanup = LogParser_cleanup;

  return bench;
}
