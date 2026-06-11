#pragma once
#include <stdint.h>

void init_serial();
void write_serial(char a);
void print_serial(const char* str);
void print_num(uint64_t val);
void print_hex(uint64_t val);