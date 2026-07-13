#include "benchmark.h"

typedef struct {
  char **s1;
  char **s2;
  size_t pair_count;
} StringPairs;

StringPairs *generate_pair_strings(int64_t n, int64_t m) {
  StringPairs *pairs = malloc(sizeof(StringPairs));
  pairs->pair_count = n;
  pairs->s1 = malloc(n * sizeof(char *));
  pairs->s2 = malloc(n * sizeof(char *));

  for (int64_t i = 0; i < n; i++) {
    int len1 = Helper_next_int(m) + 4;
    int len2 = Helper_next_int(m) + 4;

    pairs->s1[i] = malloc(len1 + 1);
    pairs->s2[i] = malloc(len2 + 1);

    for (int j = 0; j < len1; j++) {
      pairs->s1[i][j] = 'a' + Helper_next_int(10);
    }
    pairs->s1[i][len1] = '\0';

    for (int j = 0; j < len2; j++) {
      pairs->s2[i][j] = 'a' + Helper_next_int(10);
    }
    pairs->s2[i][len2] = '\0';
  }

  return pairs;
}

void free_string_pairs(StringPairs *pairs) {
  if (!pairs)
    return;

  for (size_t i = 0; i < pairs->pair_count; i++) {
    if (pairs->s1[i])
      free(pairs->s1[i]);
    if (pairs->s2[i])
      free(pairs->s2[i]);
  }
  free(pairs->s1);
  free(pairs->s2);
  free(pairs);
}

typedef struct {
  StringPairs *pairs;
  uint32_t result;
  int64_t count;
  int64_t size;
} JaroData;

void Jaro_prepare(Benchmark *self) {
  JaroData *data = (JaroData *)self->data;

  data->count = Helper_config_i64(self->name, "count");
  data->size = Helper_config_i64(self->name, "size");

  data->pairs = generate_pair_strings(data->count, data->size);
  data->result = 0;
}

double Jaro_calc(const char *s1, const char *s2) {
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);

  if (len1 == 0 || len2 == 0)
    return 0.0;

  int64_t match_dist = (len1 > len2 ? len1 : len2) / 2 - 1;
  if (match_dist < 0)
    match_dist = 0;

  bool *s1_matches = calloc(len1, sizeof(bool));
  bool *s2_matches = calloc(len2, sizeof(bool));

  int matches = 0;
  for (size_t i = 0; i < len1; i++) {
    size_t start = i > (size_t)match_dist ? i - match_dist : 0;
    size_t end = (len2 - 1 < i + match_dist) ? len2 - 1 : i + match_dist;

    for (size_t j = start; j <= end; j++) {
      if (!s2_matches[j] && s1[i] == s2[j]) {
        s1_matches[i] = true;
        s2_matches[j] = true;
        matches++;
        break;
      }
    }
  }

  double result = 0.0;

  if (matches > 0) {
    int transpositions = 0;
    size_t k = 0;

    for (size_t i = 0; i < len1; i++) {
      if (s1_matches[i]) {
        while (k < len2 && !s2_matches[k]) {
          k++;
        }
        if (k < len2) {
          if (s1[i] != s2[k]) {
            transpositions++;
          }
          k++;
        }
      }
    }
    transpositions /= 2;

    double m = (double)matches;
    result = (m / len1 + m / len2 + (m - transpositions) / m) / 3.0;
  }

  free(s1_matches);
  free(s2_matches);

  return result;
}

void Jaro_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  JaroData *data = (JaroData *)self->data;
  StringPairs *pairs = data->pairs;

  for (size_t i = 0; i < pairs->pair_count; i++) {
    double jaro = Jaro_calc(pairs->s1[i], pairs->s2[i]);
    data->result += (uint32_t)(jaro * 1000);
  }
}

uint32_t Jaro_checksum(Benchmark *self) {
  JaroData *data = (JaroData *)self->data;
  return data->result;
}

void Jaro_cleanup(Benchmark *self) {
  JaroData *data = (JaroData *)self->data;

  if (data->pairs) {
    free_string_pairs(data->pairs);
    data->pairs = NULL;
  }
}

Benchmark *Jaro_create(void) {
  Benchmark *bench = Benchmark_create("Distance::Jaro");

  JaroData *data = malloc(sizeof(JaroData));
  memset(data, 0, sizeof(JaroData));

  bench->data = data;
  bench->prepare = Jaro_prepare;
  bench->run = Jaro_run;
  bench->checksum = Jaro_checksum;
  bench->cleanup = Jaro_cleanup;

  return bench;
}

typedef struct {
  uint32_t gram;
  int count;
  UT_hash_handle hh;
} GramHash;

typedef struct {
  StringPairs *pairs;
  uint32_t result;
  int64_t count;
  int64_t size;
} NGramData;

void NGram_prepare(Benchmark *self) {
  NGramData *data = (NGramData *)self->data;

  data->count = Helper_config_i64(self->name, "count");
  data->size = Helper_config_i64(self->name, "size");

  data->pairs = generate_pair_strings(data->count, data->size);
  data->result = 0;
}

double NGram_calc(const char *s1, const char *s2) {
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);

  if (len1 < 4 || len2 < 4)
    return 0.0;

  GramHash *grams1 = NULL;

  for (size_t i = 0; i <= len1 - 4; i++) {
    uint32_t gram = ((uint32_t)(uint8_t)s1[i] << 24) |
                    ((uint32_t)(uint8_t)s1[i + 1] << 16) |
                    ((uint32_t)(uint8_t)s1[i + 2] << 8) |
                    (uint32_t)(uint8_t)s1[i + 3];

    GramHash *g;
    HASH_FIND_INT(grams1, &gram, g);
    if (g) {
      g->count++;
    } else {
      g = (GramHash *)malloc(sizeof(GramHash));
      g->gram = gram;
      g->count = 1;
      HASH_ADD_INT(grams1, gram, g);
    }
  }

  GramHash *grams2 = NULL;
  int intersection = 0;

  for (size_t i = 0; i <= len2 - 4; i++) {
    uint32_t gram = ((uint32_t)(uint8_t)s2[i] << 24) |
                    ((uint32_t)(uint8_t)s2[i + 1] << 16) |
                    ((uint32_t)(uint8_t)s2[i + 2] << 8) |
                    (uint32_t)(uint8_t)s2[i + 3];

    GramHash *g2;
    HASH_FIND_INT(grams2, &gram, g2);
    if (g2) {
      g2->count++;
    } else {
      g2 = (GramHash *)malloc(sizeof(GramHash));
      g2->gram = gram;
      g2->count = 1;
      HASH_ADD_INT(grams2, gram, g2);
    }

    GramHash *g1;
    HASH_FIND_INT(grams1, &gram, g1);
    if (g1 && g2->count <= g1->count) {
      intersection++;
    }
  }

  int total = HASH_COUNT(grams1) + HASH_COUNT(grams2);

  GramHash *current, *tmp;
  HASH_ITER(hh, grams1, current, tmp) {
    HASH_DEL(grams1, current);
    free(current);
  }
  HASH_ITER(hh, grams2, current, tmp) {
    HASH_DEL(grams2, current);
    free(current);
  }

  return total > 0 ? (double)intersection / total : 0.0;
}

void NGram_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  NGramData *data = (NGramData *)self->data;
  StringPairs *pairs = data->pairs;

  for (size_t i = 0; i < pairs->pair_count; i++) {
    double sim = NGram_calc(pairs->s1[i], pairs->s2[i]);
    data->result += (uint32_t)(sim * 1000);
  }
}

uint32_t NGram_checksum(Benchmark *self) {
  NGramData *data = (NGramData *)self->data;
  return data->result;
}

void NGram_cleanup(Benchmark *self) {
  NGramData *data = (NGramData *)self->data;

  if (data->pairs) {
    free_string_pairs(data->pairs);
    data->pairs = NULL;
  }
}

Benchmark *NGram_create(void) {
  Benchmark *bench = Benchmark_create("Distance::NGram");

  NGramData *data = malloc(sizeof(NGramData));
  memset(data, 0, sizeof(NGramData));

  bench->data = data;
  bench->prepare = NGram_prepare;
  bench->run = NGram_run;
  bench->checksum = NGram_checksum;
  bench->cleanup = NGram_cleanup;

  return bench;
}
