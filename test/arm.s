	.arch armv7-a
	.eabi_attribute 28, 1
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 6
	.eabi_attribute 34, 1
	.eabi_attribute 18, 4
	.data
	.globl a
	.type    a, %object
a:
	.word  1
	.word  1
	.space  392
	.globl b
	.type    b, %object
b:
	.word  2
	.word  1
	.space  392
	.text
	.globl main
	.syntax unified
	.type    main, %function
main:
	push  {r4, r5, fp, lr}
	sub    sp, sp, #48
	add    fp, sp, #0
	mov  r0, #0
	mov  r5,  r0
	mov r0, #2
	ldr r0, =b
	mov  r5,  r0
	str     r5, [fp, #4]
.L1:
	ldr r0, [fp, #4]
	mov r1, #0
	cmp r0,r1
	ble .L2
	ldr r0, [fp, #4]
	mov r1, #1
	sub r0, r0, r1
	str r0, [fp, #4]
	b     .L1
.L2:
	mov r0, #0
	bl putint()
	mov r0, r0
	mov sp, fp
	add  sp, sp, #48
	pop  {r4, r5, fp, pc}

