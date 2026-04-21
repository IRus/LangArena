#ifndef HELPER_H
#define HELPER_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define IM 139968
#define IA 3877
#define IC 29573
#define INIT 42

extern char **global_order;
extern size_t global_order_count;

void Helper_reset(void);
uint32_t Helper_next_int(uint32_t max);
uint32_t Helper_next_int_range(uint32_t from, uint32_t to);
double Helper_next_float(double max);

uint32_t Helper_checksum_string(const char *v);
uint32_t Helper_checksum_bytes(const uint8_t *data, size_t length);
uint32_t Helper_checksum_f64(double v);

void Helper_load_config(const char *filename);
void Helper_free_config(void);
int64_t Helper_config_i64(const char *class_name, const char *field_name);
const char *Helper_config_s(const char *class_name, const char *field_name);

#endif
