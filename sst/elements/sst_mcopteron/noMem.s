	.file	"noMem.c"
	.text
	.p2align 4,,15
.globl main
	.type	main, @function
main:
.LFB11:
	.cfi_startproc
	xorpd	%xmm0, %xmm0
	xorl	%edx, %edx
	movsd	.LC1(%rip), %xmm4
	movsd	.LC2(%rip), %xmm3
	movsd	.LC3(%rip), %xmm2
	.p2align 4,,10
	.p2align 3
.L2:
	xorl	%eax, %eax
	.p2align 4,,10
	.p2align 3
.L3:
	movapd	%xmm0, %xmm1
	cvtsi2sd %eax, %xmm0
	addl	$1, %eax
//	addsd	%xmm4, %xmm1
	cmpl	$1048576, %eax
	addsd	%xmm1, %xmm0
//	addsd	%xmm2, %xmm0
//	mulsd	%xmm1, %xmm0
	jne	.L3
	addl	$1, %edx
	cmpl	$100, %edx
	jne	.L2
	xorl	%eax, %eax
	ucomisd	.LC4(%rip), %xmm0
	seta	%al
	ret
	.cfi_endproc
.LFE11:
	.size	main, .-main
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC1:
	.long	0
	.long	1072693248
	.align 8
.LC2:
	.long	0
	.long	1073872896
	.align 8
.LC3:
	.long	0
	.long	1073741824
	.align 8
.LC4:
	.long	0
	.long	1074003968
	.ident	"GCC: (GNU) 4.4.6 20120305 (Red Hat 4.4.6-4)"
	.section	.note.GNU-stack,"",@progbits
