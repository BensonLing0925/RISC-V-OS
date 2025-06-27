#include <stdint.h>
#include "uart.h"

void uart_putchar(char ch) {
	volatile char* uart = (char*)0x10000000;
	*uart = ch;
}

void uart_print(const char* s) {
	while (*s) {
		uart_putchar(*s++);
	}
}

void uart_print_hex(uint64_t val) {
	const char* hex = "0123456789ABCDEF";
	uart_print("0x");
	for ( int i = 15 ; i >= 0 ; --i ) {
		char digit = hex[(val >> (i * 4)) & 0xF];
		uart_putchar(digit);
	}
}

void uart_print_dec(uint64_t val) {
	char buf[20] = {0};	
	int i = 0;

	if ( val == 0 ) {
		uart_putchar('0');
		return;
	}

	while (val > 0) {
		buf[i++] = '0' + (val % 10);
		val /= 10;
	}	

	while (--i >= 0) {
		uart_putchar(buf[i]);
	}
}

void uart_print_int(int64_t value) {
    if (value < 0) {
        uart_putchar('-');
        // Special case: can't negate INT64_MIN safely
        if (value == INT64_MIN) {
            uart_print_dec((uint64_t)(-(value + 1)) + 1);
            return;
        }
        value = -value;
    }
    uart_print_dec((uint64_t)value);
}

