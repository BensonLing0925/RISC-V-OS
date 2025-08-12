#ifndef RISCV_H
#define RISCV_H

#include <stdint.h>

#define read_csr(reg) ({ \
    unsigned long __tmp; \
    asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp; \
})

#define write_csr(reg, val) \
    asm volatile ("csrw " #reg ", %0" :: "r"(val))

#define set_csr(reg, val) \
    asm volatile ("csrs " #reg ", %0" :: "r"(val))

#define clear_csr(reg, val) \
    asm volatile ("csrc " #reg ", %0" :: "r"(val))

#define SSTATUS_SUM (1L << 18)


static inline uint64_t read_sstatus() {
    uint64_t x;
    asm volatile("csrr %0, sstatus" : "=r"(x));
    return x;
}

static inline void write_sstatus(uint64_t x) {
    asm volatile("csrw sstatus, %0" :: "r"(x));
}

static inline void enable_sum() {
    uint64_t sstatus = read_sstatus();
    sstatus |= SSTATUS_SUM;
    write_sstatus(sstatus);
}

static inline void disable_sum() {
    uint64_t sstatus = read_sstatus();
    sstatus &= ~SSTATUS_SUM;
    write_sstatus(sstatus);
}

#endif
