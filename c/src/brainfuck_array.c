#include "benchmark.h"

typedef struct {
  uint8_t *tape;
  size_t tape_size;
  size_t pos;
} BrainfuckArray_Tape;

typedef struct {
  char *program;
  char *warmup_program;
  int32_t program_length;
  int32_t warmup_length;
  int32_t *jumps;
  int32_t *warmup_jumps;
  uint32_t result_val;
} BrainfuckArrayData;

static void BrainfuckArray_Tape_init(BrainfuckArray_Tape *tape) {
  tape->tape_size = 30000;
  tape->tape = calloc(tape->tape_size, sizeof(uint8_t));
  tape->pos = 0;
}

static void BrainfuckArray_Tape_destroy(BrainfuckArray_Tape *tape) {
  if (tape && tape->tape) {
    free(tape->tape);
    tape->tape = NULL;
    tape->tape_size = 0;
    tape->pos = 0;
  }
}

static uint8_t BrainfuckArray_Tape_get(const BrainfuckArray_Tape *tape) {
  return tape->tape[tape->pos];
}

static void BrainfuckArray_Tape_inc(BrainfuckArray_Tape *tape) {
  tape->tape[tape->pos]++;
}

static void BrainfuckArray_Tape_dec(BrainfuckArray_Tape *tape) {
  tape->tape[tape->pos]--;
}

static void BrainfuckArray_Tape_advance(BrainfuckArray_Tape *tape) {
  tape->pos++;
  if (tape->pos >= tape->tape_size) {

    size_t new_size = tape->tape_size + 1;
    uint8_t *new_tape = realloc(tape->tape, new_size);
    if (new_tape) {
      tape->tape = new_tape;
      tape->tape[tape->tape_size] = 0;
      tape->tape_size = new_size;
    }
  }
}

static void BrainfuckArray_Tape_devance(BrainfuckArray_Tape *tape) {
  if (tape->pos > 0) {
    tape->pos--;
  }
}

static char *BrainfuckArray_filter_commands(const char *input,
                                            int32_t *out_length) {
  if (!input) {
    *out_length = 0;
    return NULL;
  }

  size_t input_len = strlen(input);
  char *program = malloc(input_len + 1);
  if (!program) {
    *out_length = 0;
    return NULL;
  }

  int32_t program_pos = 0;
  for (size_t i = 0; i < input_len; i++) {
    char c = input[i];
    if (strchr("[]<>+-,.", c)) {
      program[program_pos++] = c;
    }
  }
  program[program_pos] = '\0';

  *out_length = program_pos;
  return program;
}

static int32_t *BrainfuckArray_build_jumps(const char *program,
                                           int32_t program_length) {
  if (!program || program_length == 0)
    return NULL;

  int32_t *jumps = calloc(program_length, sizeof(int32_t));
  if (!jumps)
    return NULL;

  int32_t *stack = malloc(sizeof(int32_t) * (program_length / 2 + 1));
  if (!stack) {
    free(jumps);
    return NULL;
  }

  int32_t stack_top = -1;

  for (int32_t pc = 0; pc < program_length; pc++) {
    char c = program[pc];
    if (c == '[') {
      stack[++stack_top] = pc;
    } else if (c == ']') {
      if (stack_top >= 0) {
        int32_t left = stack[stack_top--];
        jumps[left] = pc;
        jumps[pc] = left;
      }
    }
  }

  free(stack);
  return jumps;
}

static uint32_t BrainfuckArray_execute_program(const char *program,
                                               int32_t program_length,
                                               const int32_t *jumps) {
  if (!program || program_length == 0 || !jumps) {
    return 0;
  }

  BrainfuckArray_Tape tape;
  BrainfuckArray_Tape_init(&tape);

  int32_t pc = 0;
  uint32_t result = 0;

  while (pc < program_length) {
    char c = program[pc];
    switch (c) {
    case '+':
      BrainfuckArray_Tape_inc(&tape);
      break;

    case '-':
      BrainfuckArray_Tape_dec(&tape);
      break;

    case '>':
      BrainfuckArray_Tape_advance(&tape);
      break;

    case '<':
      BrainfuckArray_Tape_devance(&tape);
      break;

    case '[':
      if (BrainfuckArray_Tape_get(&tape) == 0) {
        pc = jumps[pc];
      }
      break;

    case ']':
      if (BrainfuckArray_Tape_get(&tape) != 0) {
        pc = jumps[pc];
      }
      break;

    case '.':
      result = (result << 2) + BrainfuckArray_Tape_get(&tape);
      break;

    default:
      break;
    }
    pc++;
  }

  BrainfuckArray_Tape_destroy(&tape);
  return result;
}

void BrainfuckArray_prepare(Benchmark *self) {
  BrainfuckArrayData *data = (BrainfuckArrayData *)self->data;

  const char *program_text = Helper_config_s(self->name, "program");
  const char *warmup_text = Helper_config_s(self->name, "warmup_program");

  data->program =
      BrainfuckArray_filter_commands(program_text, &data->program_length);
  data->jumps = BrainfuckArray_build_jumps(data->program, data->program_length);

  data->warmup_program =
      BrainfuckArray_filter_commands(warmup_text, &data->warmup_length);
  data->warmup_jumps =
      BrainfuckArray_build_jumps(data->warmup_program, data->warmup_length);

  data->result_val = 0;
}

void BrainfuckArray_warmup(Benchmark *self) {
  BrainfuckArrayData *data = (BrainfuckArrayData *)self->data;

  if (!data->warmup_program || data->warmup_length == 0)
    return;

  int64_t warmup_iters = Helper_config_i64(self->name, "warmup_iterations");
  if (warmup_iters == 0) {
    warmup_iters = self->iterations(self) / 5;
    if (warmup_iters < 1)
      warmup_iters = 1;
  }

  for (int64_t i = 0; i < warmup_iters; i++) {
    BrainfuckArray_execute_program(data->warmup_program, data->warmup_length,
                                   data->warmup_jumps);
  }
}

void BrainfuckArray_run(Benchmark *self, int iteration_id) {
  BrainfuckArrayData *data = (BrainfuckArrayData *)self->data;

  if (iteration_id == 0) {
    data->result_val = 0;
  }

  if (data->program && data->program_length > 0 && data->jumps) {
    uint32_t run_result = BrainfuckArray_execute_program(
        data->program, data->program_length, data->jumps);
    data->result_val += run_result;
  }
}

uint32_t BrainfuckArray_checksum(Benchmark *self) {
  BrainfuckArrayData *data = (BrainfuckArrayData *)self->data;
  return data->result_val;
}

void BrainfuckArray_cleanup(Benchmark *self) {
  if (!self || !self->data)
    return;

  BrainfuckArrayData *data = (BrainfuckArrayData *)self->data;

  free(data->program);
  free(data->jumps);
  free(data->warmup_program);
  free(data->warmup_jumps);

  free(data);
  self->data = NULL;
}

Benchmark *BrainfuckArray_create(void) {
  Benchmark *bench = Benchmark_create("Brainfuck::Array");
  if (!bench)
    return NULL;

  BrainfuckArrayData *data = calloc(1, sizeof(BrainfuckArrayData));
  if (!data) {
    free(bench);
    return NULL;
  }

  bench->data = data;
  bench->prepare = BrainfuckArray_prepare;
  bench->warmup = BrainfuckArray_warmup;
  bench->run = BrainfuckArray_run;
  bench->checksum = BrainfuckArray_checksum;
  bench->cleanup = BrainfuckArray_cleanup;

  return bench;
}
