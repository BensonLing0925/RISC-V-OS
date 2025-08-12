#include "riscv.h"

#define SSTATUS_SUM (1L << 18)

// Inline functions for accessing CSRs
inline uint64_t read_csr(const char* csr_name) {
    uint64_t value;
    asm volatile ("csrr %0, %1" : "=r"(value) : "i"(csr_name));
    return value;
}

inline void write_csr(const char* csr_name, uint64_t value) {
    asm volatile ("csrw %0, %1" :: "i"(csr_name), "r"(value));
}

inline uint64_t read_sstatus() {
    uint64_t x;
    asm volatile("csrr %0, sstatus" : "=r"(x));
    return x;
}

inline void write_sstatus(uint64_t x) {
    asm volatile("csrw sstatus, %0" :: "r"(x));
}

inline void enable_sum() {
    write_csr(sstatus, read_csr(sstatus) | SSTATUS_SUM);
}

inline void disable_sum() {
    write_csr(sstatus, read_csr(sstatus) & ~SSTATUS_SUM);
}
