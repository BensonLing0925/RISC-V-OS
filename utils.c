#include <stdint.h>
#include <stdarg.h>
#include "uart.h"
#include "utils.h"

void debug_print_sp(char* top) {
	register uintptr_t sp asm("sp");
	utils_printf("Raw SP = %x\n", sp);
	utils_printf("Stack top = %x\n", top);
}

void utils_printf(const char* format, ...) {

	va_list args;
	va_start(args, format);

	while (*format) {
		if (*format == '%') {
			format++;
			char ch = *format;
			switch (ch) {
				case 'c': {
					char c = (char)va_arg(args, int);
					uart_putchar(c);
					break;
				}
				case 's': {
					char* s = va_arg(args, char*);
					uart_print(s);
					break;
				} // case 's'
				case 'x': {
					uint64_t val = va_arg(args, uint64_t);
					uart_print_hex(val);
					break;
				} // case 'x'
				case 'u': {
					uint64_t val = va_arg(args, uint64_t);
					uart_print_dec(val);		
					break;
				} // case 'u'
				case 'd' : {
					int64_t val = va_arg(args, int64_t);
					uart_print_int(val);
					break;
				} // case 'd'
				default:
					uart_putchar('%');
					uart_putchar(ch);
					break;
			} // switch
			format++;
		} // if
		else {
			uart_putchar(*format++);
		} // else
	}
	va_end(args);
}
