CROSS = riscv64-unknown-elf
CFLAGS = -O0 -g -ffreestanding -nostdlib -mabi=lp64 -march=rv64g -mcmodel=medany
OBJS = start.o uart.o utils.o init.o trap.o trap_c.o smain.o mem.o s_trap.o syscall.o exec.o umain.o

all: kernel.elf

kernel.elf: $(OBJS) linker.ld
	$(CROSS)-ld -T linker.ld -o $@ $(OBJS)

%.o: %.c
	$(CROSS)-gcc $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CROSS)-gcc $(CFLAGS) -c $< -o $@

trap.o: trap.S
	$(CROSS)-gcc $(CFLAGS) -c $< -o $@

trap_c.o: trap.c
	$(CROSS)-gcc $(CFLAGS) -c $< -o $@

run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel.elf

load: 
	riscv64-unknown-elf-gcc -nostdlib -static -o umain.elf umain.c
	riscv64-unknown-elf-objcopy -O binary umain.elf user.bin
	xxd -i user.bin > user_program.h

gdb:
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel.elf -S -gdb tcp::1234

dump:
	riscv64-unknown-elf-objdump -D kernel.elf > dump.txt

clean:
	rm -f *.o kernel.elf
