.extern zero_bss
.extern init_trap
.extern jump_to_smode

.section .text
.globl _start
_start:
    # Set up M-mode stack
    la sp, stack_top
    
    # Initialize BSS section
    call zero_bss
    
    # Set up M-mode trap handler
    call init_trap
    
    call init_vm
    
    # Should never reach here
1:  j 1b

.section .bss
.balign 16
stack:
    .space 4096
.globl stack_top
stack_top:

# S-mode stack (separate from M-mode stack)
.balign 16
sstack:
    .space 4096
.globl sstack_top
sstack_top:

.globl sstack_bottom  
sstack_bottom:
