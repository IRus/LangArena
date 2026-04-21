#include "benchmark.h"

typedef enum {
  BrainfuckRecursion_OP_INC,
  BrainfuckRecursion_OP_DEC,
  BrainfuckRecursion_OP_RIGHT,
  BrainfuckRecursion_OP_LEFT,
  BrainfuckRecursion_OP_PRINT,
  BrainfuckRecursion_OP_LOOP
} BrainfuckRecursion_OpType;

typedef struct BrainfuckRecursion_Op BrainfuckRecursion_Op;

struct BrainfuckRecursion_Op {
  BrainfuckRecursion_OpType type;
  BrainfuckRecursion_Op *loop_ops;
  int32_t loop_size;
};

typedef struct BrainfuckRecursion_Tape {
  uint8_t *tape;
  int32_t size;
  int32_t pos;
} BrainfuckRecursion_Tape;

static BrainfuckRecursion_Tape *BrainfuckRecursion_Tape_new(void) {
  BrainfuckRecursion_Tape *self = malloc(sizeof(BrainfuckRecursion_Tape));
  self->size = 30000;
  self->tape = calloc(self->size, sizeof(uint8_t));
  self->pos = 0;
  return self;
}

static uint8_t BrainfuckRecursion_Tape_get(BrainfuckRecursion_Tape *self) {
  return self->tape[self->pos];
}

static void BrainfuckRecursion_Tape_inc(BrainfuckRecursion_Tape *self) {
  self->tape[self->pos]++;
}

static void BrainfuckRecursion_Tape_dec(BrainfuckRecursion_Tape *self) {
  self->tape[self->pos]--;
}

static void BrainfuckRecursion_Tape_right(BrainfuckRecursion_Tape *self) {
  self->pos++;
  if (self->pos >= self->size) {
    self->size++;
    self->tape = realloc(self->tape, self->size);
    self->tape[self->size - 1] = 0;
  }
}

static void BrainfuckRecursion_Tape_left(BrainfuckRecursion_Tape *self) {
  if (self->pos > 0) {
    self->pos--;
  }
}

static void BrainfuckRecursion_Tape_free(BrainfuckRecursion_Tape *self) {
  free(self->tape);
  free(self);
}

static BrainfuckRecursion_Op *BrainfuckRecursion_parse_ops(const char **code,
                                                           int32_t *ops_count) {
  int32_t capacity = 16;
  BrainfuckRecursion_Op *ops = malloc(sizeof(BrainfuckRecursion_Op) * capacity);
  int32_t count = 0;

  while (**code) {
    if (count >= capacity) {
      capacity *= 2;
      ops = realloc(ops, sizeof(BrainfuckRecursion_Op) * capacity);
    }

    switch (**code) {
    case '+':
      ops[count].type = BrainfuckRecursion_OP_INC;
      ops[count].loop_ops = NULL;
      ops[count].loop_size = 0;
      count++;
      break;
    case '-':
      ops[count].type = BrainfuckRecursion_OP_DEC;
      ops[count].loop_ops = NULL;
      ops[count].loop_size = 0;
      count++;
      break;
    case '>':
      ops[count].type = BrainfuckRecursion_OP_RIGHT;
      ops[count].loop_ops = NULL;
      ops[count].loop_size = 0;
      count++;
      break;
    case '<':
      ops[count].type = BrainfuckRecursion_OP_LEFT;
      ops[count].loop_ops = NULL;
      ops[count].loop_size = 0;
      count++;
      break;
    case '.':
      ops[count].type = BrainfuckRecursion_OP_PRINT;
      ops[count].loop_ops = NULL;
      ops[count].loop_size = 0;
      count++;
      break;
    case '[':
      (*code)++;
      ops[count].type = BrainfuckRecursion_OP_LOOP;
      ops[count].loop_ops = NULL;
      ops[count].loop_size = 0;

      int32_t loop_ops_count = 0;
      ops[count].loop_ops = BrainfuckRecursion_parse_ops(code, &loop_ops_count);
      ops[count].loop_size = loop_ops_count;

      count++;
      continue;
    case ']':
      *ops_count = count;
      (*code)++;
      return ops;
    default:
      break;
    }
    (*code)++;
  }

  *ops_count = count;
  return ops;
}

static void BrainfuckRecursion_free_ops(BrainfuckRecursion_Op *ops,
                                        int32_t ops_size) {
  if (!ops)
    return;

  for (int32_t i = 0; i < ops_size; i++) {
    if (ops[i].type == BrainfuckRecursion_OP_LOOP && ops[i].loop_ops) {
      BrainfuckRecursion_free_ops(ops[i].loop_ops, ops[i].loop_size);
    }
  }
  free(ops);
}

static void BrainfuckRecursion_run_ops(BrainfuckRecursion_Op *ops,
                                       int32_t ops_size,
                                       BrainfuckRecursion_Tape *tape,
                                       uint32_t *result) {
  for (int32_t i = 0; i < ops_size; i++) {
    BrainfuckRecursion_Op *op = &ops[i];
    switch (op->type) {
    case BrainfuckRecursion_OP_INC:
      BrainfuckRecursion_Tape_inc(tape);
      break;
    case BrainfuckRecursion_OP_DEC:
      BrainfuckRecursion_Tape_dec(tape);
      break;
    case BrainfuckRecursion_OP_RIGHT:
      BrainfuckRecursion_Tape_right(tape);
      break;
    case BrainfuckRecursion_OP_LEFT:
      BrainfuckRecursion_Tape_left(tape);
      break;
    case BrainfuckRecursion_OP_PRINT: {
      uint8_t value = BrainfuckRecursion_Tape_get(tape);
      *result = (*result << 2) + value;
      break;
    }
    case BrainfuckRecursion_OP_LOOP:
      while (BrainfuckRecursion_Tape_get(tape) != 0) {
        BrainfuckRecursion_run_ops(op->loop_ops, op->loop_size, tape, result);
      }
      break;
    }
  }
}

static uint32_t BrainfuckRecursion_run_program(const char *code) {
  if (!code || !code[0]) {
    return 0;
  }

  const char *code_ptr = code;
  int32_t ops_count = 0;
  BrainfuckRecursion_Op *ops =
      BrainfuckRecursion_parse_ops(&code_ptr, &ops_count);

  if (!ops) {
    return 0;
  }

  BrainfuckRecursion_Tape *tape = BrainfuckRecursion_Tape_new();
  uint32_t result = 0;
  BrainfuckRecursion_run_ops(ops, ops_count, tape, &result);

  BrainfuckRecursion_Tape_free(tape);
  BrainfuckRecursion_free_ops(ops, ops_count);

  return result;
}

typedef struct {
  const char *program;
  const char *warmup_program;
  uint32_t result_val;
} BrainfuckRecursionData;

void BrainfuckRecursion_prepare(Benchmark *self) {
  BrainfuckRecursionData *data = (BrainfuckRecursionData *)self->data;
  data->program = Helper_config_s(self->name, "program");
  data->warmup_program = Helper_config_s(self->name, "warmup_program");
  data->result_val = 0;
}

void BrainfuckRecursion_warmup(Benchmark *self) {
  BrainfuckRecursionData *data = (BrainfuckRecursionData *)self->data;
  int64_t warmup_iters = Helper_config_i64(self->name, "warmup_iterations");

  if (warmup_iters == 0) {
    warmup_iters = self->iterations(self);
    warmup_iters = (int64_t)(warmup_iters * 0.2);
    if (warmup_iters < 1)
      warmup_iters = 1;
  }

  for (int64_t i = 0; i < warmup_iters; i++) {
    BrainfuckRecursion_run_program(data->warmup_program);
  }
}

void BrainfuckRecursion_run(Benchmark *self, int iteration_id) {
  BrainfuckRecursionData *data = (BrainfuckRecursionData *)self->data;

  if (iteration_id == 0) {
    data->result_val = 0;
  }

  uint32_t run_result = BrainfuckRecursion_run_program(data->program);
  data->result_val += run_result;
}

uint32_t BrainfuckRecursion_checksum(Benchmark *self) {
  BrainfuckRecursionData *data = (BrainfuckRecursionData *)self->data;
  return data->result_val;
}

void BrainfuckRecursion_cleanup(Benchmark *self) { (void)self; }

Benchmark *BrainfuckRecursion_create(void) {
  Benchmark *bench = Benchmark_create("Brainfuck::Recursion");

  BrainfuckRecursionData *data = malloc(sizeof(BrainfuckRecursionData));
  data->program = NULL;
  data->warmup_program = NULL;
  data->result_val = 0;

  bench->data = data;
  bench->prepare = BrainfuckRecursion_prepare;
  bench->warmup = BrainfuckRecursion_warmup;
  bench->run = BrainfuckRecursion_run;
  bench->checksum = BrainfuckRecursion_checksum;
  bench->cleanup = BrainfuckRecursion_cleanup;

  return bench;
}
