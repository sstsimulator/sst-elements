	.machine ppc
	.const
	.align 2
LC0:
	.ascii "Hello world!\0"
	.text
	.align 2
	.globl _main
_main:
	mflr r0
	stw r0,8(r1)
	stw r30,-8(r1)
	stw r31,-4(r1)
	stwu r1,-80(r1)
	mr r30,r1
	bcl 20,31,"L00000000001$pb"
"L00000000001$pb":
	mflr r31
	addis r2,r31,ha16(LC0-"L00000000001$pb")
	mr r0,r2
	addic r3,r0,lo16(LC0-"L00000000001$pb")
	bl L_puts$stub
	li r0,0
	mr r3,r0
	addi r1,r30,80
	lwz r0,8(r1)
	mtlr r0
	lwz r30,-8(r1)
	lwz r31,-4(r1)
	blr
	.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 5
L_puts$stub:
	.indirect_symbol _puts
	mflr r0
	bcl 20,31,"L00000000001$spb"
"L00000000001$spb":
	mflr r11
	addis r11,r11,ha16(L_puts$lazy_ptr-"L00000000001$spb")
	mtlr r0
	lwzu r12,lo16(L_puts$lazy_ptr-"L00000000001$spb")(r11)
	mtctr r12
	bctr
	.lazy_symbol_pointer
L_puts$lazy_ptr:
	.indirect_symbol _puts
	.long	dyld_stub_binding_helper
	.subsections_via_symbols
