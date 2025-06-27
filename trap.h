#ifndef TRAP_H
#define TRAP_H

#include <stdint.h>

struct trapframe {
	uint64_t ra;
	uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
	uint64_t t0, t1, t2, t3, t4, t5, t6;
	uint64_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
	uint64_t sp;
	uint64_t gp;
	uint64_t tp;

	// CSR state
	uint64_t sepc;
	uint64_t sstatus;
	uint64_t satp;
};



#endif
