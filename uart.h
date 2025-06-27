
#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_putchar(char ch);
void uart_print(const char* s);
void uart_print_hex(uint64_t val);
void uart_print_dec(uint64_t val);
void uart_print_int(int64_t val);

#endif
