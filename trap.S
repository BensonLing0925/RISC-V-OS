.section .text.trap
.global trap_entry
trap_entry:
	addi sp, sp, -256
	
	sd ra, 0(sp)
	sd a0, 8(sp)
	sd a1, 16(sp)
	sd a2, 24(sp)
	sd a3, 32(sp)
	sd a4, 40(sp)
	sd a5, 48(sp)
	sd a6, 56(sp)
	sd a7, 64(sp)
	sd t0, 72(sp)
	sd t1, 80(sp)
	sd t2, 88(sp)
	sd t3, 96(sp)
	sd t4, 104(sp)
	sd t5, 112(sp)
	sd t6, 120(sp)
	sd s0, 128(sp)
	sd s1, 136(sp)
	sd s2, 144(sp)
	sd s3, 152(sp)
	sd s4, 160(sp)
	sd s5, 168(sp)
	sd s6, 176(sp)
	sd s7, 184(sp)
	sd s8, 192(sp)
	sd s9, 200(sp)
	sd s10, 208(sp)
	sd s11, 216(sp)

	call handle_trap

	ld ra, 0(sp)
	ld a0, 8(sp)
	ld a1, 16(sp)
	ld a2, 24(sp)
	ld a3, 32(sp)
	ld a4, 40(sp)
	ld a5, 48(sp)
	ld a6, 56(sp)
	ld a7, 64(sp)
	ld t0, 72(sp)
	ld t1, 80(sp)
	ld t2, 88(sp)
	ld t3, 96(sp)
	ld t4, 104(sp)
	ld t5, 112(sp)
	ld t6, 120(sp)
	ld s0, 128(sp)
	ld s1, 136(sp)
	ld s2, 144(sp)
	ld s3, 152(sp)
	ld s4, 160(sp)
	ld s5, 168(sp)
	ld s6, 176(sp)
	ld s7, 184(sp)
	ld s8, 192(sp)
	ld s9, 200(sp)
	ld s10, 208(sp)
	ld s11, 216(sp)

	addi sp, sp, 256
	mret

.global s_trap_entry
s_trap_entry:

	csrrw sp, sscratch, sp

	addi sp, sp, -272
	
	sd ra, 0(sp)
	sd a0, 8(sp)
	sd a1, 16(sp)
	sd a2, 24(sp)
	sd a3, 32(sp)
	sd a4, 40(sp)
	sd a5, 48(sp)
	sd a6, 56(sp)
	sd a7, 64(sp)
	sd t0, 72(sp)
	sd t1, 80(sp)
	sd t2, 88(sp)
	sd t3, 96(sp)
	sd t4, 104(sp)
	sd t5, 112(sp)
	sd t6, 120(sp)
	sd s0, 128(sp)
	sd s1, 136(sp)
	sd s2, 144(sp)
	sd s3, 152(sp)
	sd s4, 160(sp)
	sd s5, 168(sp)
	sd s6, 176(sp)
	sd s7, 184(sp)
	sd s8, 192(sp)
	sd s9, 200(sp)
	sd s10, 208(sp)
	sd s11, 216(sp)
	sd sp, 224(sp)
	sd gp, 232(sp)
	sd tp, 240(sp)
	
	csrr t0, sepc
	sd t0, 248(sp)
	csrr t0, sstatus
	sd t0, 256(sp)
	csrr t0, satp
	sd t0, 264(sp)

	mv a0, sp	// a0 points to struct trap_frame

	call s_handle_trap

	ld ra, 0(sp)
	ld a0, 8(sp)
	ld a1, 16(sp)
	ld a2, 24(sp)
	ld a3, 32(sp)
	ld a4, 40(sp)
	ld a5, 48(sp)
	ld a6, 56(sp)
	ld a7, 64(sp)
	ld t0, 72(sp)
	ld t1, 80(sp)
	ld t2, 88(sp)
	ld t3, 96(sp)
	ld t4, 104(sp)
	ld t5, 112(sp)
	ld t6, 120(sp)
	ld s0, 128(sp)
	ld s1, 136(sp)
	ld s2, 144(sp)
	ld s3, 152(sp)
	ld s4, 160(sp)
	ld s5, 168(sp)
	ld s6, 176(sp)
	ld s7, 184(sp)
	ld s8, 192(sp)
	ld s9, 200(sp)
	ld s10, 208(sp)
	ld s11, 216(sp)
	ld sp, 224(sp)
	ld gp, 232(sp)
	ld tp, 240(sp)

	ld t0, 248(sp)
	csrw sepc, t0
	ld t0, 256(sp)
	csrw sstatus, t0
	ld t0, 264(sp)
	csrw satp, t0

	addi sp, sp, 272

	csrrw sp, sscratch, sp

	sret
