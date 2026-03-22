	.file	"<stdin>"
	.text
	.section	__TEXT,__cstring,cstring_literals
l_.str0:
	.asciz	"%f %f\n"
l_.str1:
	.asciz	"%f %f\n"
	.text

	.globl	_recursive_divider
	.p2align	2
	_recursive_divider:
	stp	x29, x30, [sp, -16]!
	mov	x29, sp
	sub	sp, sp, #32
	adrp	x0, l_.str0@PAGE
	add	x0, x0, l_.str0@PAGEOFF
	mov	x8, x0
	mov	x9, sp
	str	x8, [x9]
	bl	_printf
	add	sp, sp, #32
	mov	x8, x0
	mov	x0, x0
	mov	x1, x1
	sdiv	w0, w0, w1
	mov	x8, x0
	sub	sp, sp, #32
	adrp	x0, l_.str1@PAGE
	add	x0, x0, l_.str1@PAGEOFF
	mov	x8, x0
	mov	x9, sp
	str	x8, [x9]
	bl	_printf
	add	sp, sp, #32
	mov	x8, x0
	mov	x8, x0
	mov	x0, x0
	mov	x1, x1
	bl	_recursive_divider
	mov	x8, x0
	mov	x0, x8
	ldp	x29, x30, [sp], 16
	ret

	.globl	_main
	.p2align	2
	_main:
	.globl	main
	.set	main, _main
	stp	x29, x30, [sp, -16]!
	mov	x29, sp
	mov	x8, x0
	mov	x0, xzr
	mov	x1, xzr
	bl	_recursive_divider
	mov	x8, x0
	mov	x0, #0
	ldp	x29, x30, [sp], 16
	ret
