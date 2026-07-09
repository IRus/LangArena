#include "benchmark.h"

typedef struct {
  int64_t words;
  int64_t word_len;
  char *text;
  size_t text_size;
  uint32_t checksum_val;
} WordsData;

static void WordsData_init(WordsData *data) {
  data->words = Helper_config_i64("Etc::Words", "words");
  data->word_len = Helper_config_i64("Etc::Words", "word_len");
  data->text = NULL;
  data->text_size = 0;
  data->checksum_val = 0;
}

void Words_prepare(Benchmark *self) {
  WordsData *data = (WordsData *)self->data;

  if (data->text) {
    free(data->text);
    data->text = NULL;
  }

  const char *chars = "abcdefghijklmnopqrstuvwxyz";
  int char_count = 26;

  char **words_array = malloc(data->words * sizeof(char *));
  size_t total_len = 0;

  for (int i = 0; i < (int)data->words; i++) {
    int len = Helper_next_int((int)data->word_len);
    len += Helper_next_int(3) + 3;
    char *word = malloc(len + 1);
    for (int j = 0; j < len; j++) {
      int idx = Helper_next_int(char_count);
      word[j] = chars[idx];
    }
    word[len] = '\0';
    words_array[i] = word;
    total_len += len + 1;
  }

  char *buffer = malloc(total_len + 1);
  size_t pos = 0;
  for (int i = 0; i < (int)data->words; i++) {
    size_t word_len = strlen(words_array[i]);
    memcpy(buffer + pos, words_array[i], word_len);
    pos += word_len;
    if (i < data->words - 1) {
      buffer[pos++] = ' ';
    }
    free(words_array[i]);
  }
  buffer[pos] = '\0';
  free(words_array);

  data->text = buffer;
  data->text_size = pos;
}

typedef struct {
  char *word;
  int count;
  UT_hash_handle hh;
} WordEntry;

void Words_run(Benchmark *self, int iteration_id) {
  (void)iteration_id;
  WordsData *data = (WordsData *)self->data;

  WordEntry *frequencies = NULL;

  char *text_copy = strdup(data->text);
  char *saveptr;
  char *word = strtok_r(text_copy, " ", &saveptr);

  while (word != NULL) {
    if (strlen(word) >= 3) {
      WordEntry *entry;
      HASH_FIND_STR(frequencies, word, entry);
      if (entry) {
        entry->count++;
      } else {
        entry = malloc(sizeof(WordEntry));
        entry->word = strdup(word);
        entry->count = 1;
        HASH_ADD_STR(frequencies, word, entry);
      }
    }
    word = strtok_r(NULL, " ", &saveptr);
  }

  free(text_copy);

  char max_word[256] = "";
  int max_count = 0;
  WordEntry *entry, *tmp;

  HASH_ITER(hh, frequencies, entry, tmp) {
    if (entry->count > max_count) {
      max_count = entry->count;
      strncpy(max_word, entry->word, 255);
      max_word[255] = '\0';
    }
  }

  int freq_size = HASH_COUNT(frequencies);
  uint32_t word_checksum = Helper_checksum_string(max_word);

  data->checksum_val +=
      (uint32_t)max_count + word_checksum + (uint32_t)freq_size;

  HASH_ITER(hh, frequencies, entry, tmp) {
    HASH_DEL(frequencies, entry);
    free(entry->word);
    free(entry);
  }
}

uint32_t Words_checksum(Benchmark *self) {
  WordsData *data = (WordsData *)self->data;
  return data->checksum_val;
}

void Words_cleanup(Benchmark *self) {
  WordsData *data = (WordsData *)self->data;
  if (data->text) {
    free(data->text);
    data->text = NULL;
  }
  data->text_size = 0;
}

Benchmark *Words_create(void) {
  Benchmark *bench = Benchmark_create("Etc::Words");

  WordsData *data = calloc(1, sizeof(WordsData));
  WordsData_init(data);

  bench->data = data;
  bench->prepare = Words_prepare;
  bench->run = Words_run;
  bench->checksum = Words_checksum;
  bench->cleanup = Words_cleanup;

  return bench;
}
