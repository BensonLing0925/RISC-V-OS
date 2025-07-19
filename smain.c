#include <stdint.h>
#include "user_program.h"
#include "utils.h"
#include "exec.h"

extern uintptr_t* user_pagetable;
extern void jump_to_umode(uintptr_t user_prog_entry, uintptr_t* pagetable);
extern void jump_to_umode_debug(uintptr_t user_prog_entry, uintptr_t* pagetable);
extern uintptr_t load_user_program(const char* user_program, size_t program_size);



void smain() {
    // CRITICAL: First instruction - set up stack pointer from sscratch
    // asm volatile("csrw satp, zero");
    // Immediate debug output to confirm we reached smain
    utils_printf("=== SMAIN ENTERED ===\n");
    utils_printf("smain: Successfully entered S-mode!\n");

	uint64_t syscall = 2;
	const char* msg = "This is a test";
	asm volatile("mv a7, %0" :: "r"(syscall));
	asm volatile("mv a0, %0" :: "r"(msg));
	asm volatile("ecall");

	uintptr_t user_program_entry = load_user_program((const char*)user_bin, user_bin_len);	
	jump_to_umode(user_program_entry, user_pagetable);
	// minimal_page_table_switch_test(user_pagetable);
	// jump_to_umode_no_stack_change(user_program_entry, user_pagetable);
}
