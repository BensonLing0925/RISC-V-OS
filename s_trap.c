#include <stdint.h>
#include "syscall.h"
#include "trap.h"
#include "utils.h"

struct trapframe kframe;

void s_handle_trap(struct trapframe* kframe) {

	uintptr_t scause;
	uintptr_t stval;
	uintptr_t sepc;

	asm volatile("csrr %0, scause" : "=r"(scause));
	asm volatile("csrr %0, sepc" : "=r"(sepc));
	asm volatile("csrr %0, stval" : "=r"(stval));

	utils_printf("[s-trap] scause: %x\n", scause);
	utils_printf("[s-trap] sepc: %x\n", sepc);
	utils_printf("[s-trap] stval: %x\n", stval);

	// exception
	if ((scause >> 63) == 0) {
		switch (scause) {
			case 0x8:		// ecall from U mode 
				utils_printf("[s-trap] Environment call from U mode (ecall)\n");
				sepc += 4;
				asm volatile("csrw sepc, %0" :: "r"(sepc));
				syscall_dispatch(kframe->a7, kframe->a0, kframe->a1, kframe->a2);
				break;

			case 0x9:		// ecall from S mode 
				utils_printf("[s-trap] Environment call from S mode (ecall)\n");

				syscall_dispatch(kframe->a7, kframe->a0, kframe->a1, kframe->a2);

				sepc += 4;
				asm volatile("csrw sepc, %0" :: "r"(sepc));
				break;

			default:  // 🔥 ADD THIS
				utils_printf("[s-trap] Unhandled trap!\n");
				utils_printf("scause: %x\n", scause);
				utils_printf("sepc:   %x\n", sepc);
				utils_printf("stval:  %x\n", stval);
				while (1);  // halt
		}


	}
	// interrupt (bit 63 is 1)
	else {
		switch (scause & 0xfff) {
			case 0x5:		// supervisor timer interrupt
				break;

			case 0x9:		// external IRQ
				break;
		}
	}
	asm volatile("sret");
}
