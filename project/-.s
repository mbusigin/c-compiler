	.file	"<stdin>"
	.text

	.globl	_f
	.p2align	2
	_f:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]
	mov	x29, sp
	add	x29, x29, #16
	str	x0, [sp, #0]
	mov	x10, x8
	ldr	x0, [sp, #0]
	mov	x8, x0
	ldr	w8, [x8]
	mov	x21, x9
	mov	x10, x8
	ldr	x0, [sp, #0]
	mov	x8, x0
	mov	x9, x21
	mov	x0, x10
	mov	x1, x8
	add	w0, w0, w1
	mov	x8, x0
	ldr	w8, [x8]
	mov	x0, x10
	mov	x1, x8
	add	w0, w0, w1
	mov	x8, x0
	mov	x0, x8
	ldp	x29, x30, [x29]
	add	sp, sp, #32
	ret
