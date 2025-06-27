#include "kernel.h"
#include "uart.h"
#include "utils.h"

struct CPU cpu_data[MAX_CPU];

void init_cpu() {
	for ( int i = 0 ; i < MAX_CPU ; ++i ) {
		cpu_data[i].id = i;
		cpu_data[i].tf = NULL;
	}
}

void* get_kernel_stack_top(int cpu_id) {
	return cpu_data[cpu_id].kstack;
}

int get_cpu_id() {
	return 0;     // return 0 for now
}
