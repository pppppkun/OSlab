/* Real Mode Hello World */
.code16

start:
	cli
	inb $0x92, %al
	orb $0x02, %al
	outb %al, $0x92
	data32 addr32 lgdt gdtDesc
	movl %cr0, %eax
	orb $0x01, %al
	movl %eax, %cr0
	data32 ljmp $0x08, $start32
/* 	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $0x7d00, %ax
	movw %ax, %sp # setting stack pointer to 0x7d00
	pushw $13 # pushing the size to print into stack
	pushw $message # pushing the address of message into stack
	callw displayStr # calling the display function */

.code32
start32:
	movw $0x10,%ax
	movw %ax,%ds
	movw %ax,%ss
	movw %ax,%fs
	movw $0x18,%ax
	movw %ax,%gs
	movw $0x8c00,%eax
	movw %eax,%esp
	jmp bootMain

gdt:
	.word 0,0
	.byte 0,0,0,0
	
	.word 0xffff,0
	.byte 0,0x9a,0xcf,0

	.word 0xffff,0
	.byte 0,0x92,0xcf,0

	.word 0xffff,0x8000
	.byte 0x0b,0x92,0xcf,0

gdtDesc:
	.word (gdtDesc - gdt - 1)
	.long gdt

/* loop:
	jmp loop

message:
	.string "Hello, World!\n\0"

displayStr:
	pushw %bp
	movw 4(%esp), %ax
	movw %ax, %bp
	movw 6(%esp), %cx
	movw $0x1301, %ax
	movw $0x000c, %bx
	movw $0x0000, %dx
	int $0x10
	popw %bp
	ret
*/