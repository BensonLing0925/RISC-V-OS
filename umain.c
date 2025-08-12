const char msg[] = "Hello from user program!\n";

void _start() {
    asm volatile (
        "la a0, msg\n"
        "li a7, 2\n"
        "ecall\n"
    );
	while (1);
}

