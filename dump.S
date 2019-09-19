.global dump_state
dump_state:
	cli

	push	r31
	push	r30
	push	r29
	push	r28
	push	r27
	push	r26
	push	r25
	push	r24
	push	r23
	push	r22
	push	r21
	push	r20
	push	r19
	push	r18
	push	r17
	push	r16
	push	r15
	push	r14
	push	r13
	push	r12
	push	r11
	push	r10
	push	r9
	push	r8
	push	r7
	push	r6
	push	r5
	push	r4
	push	r3
	push	r2
	push	r1
	push	r0


	ldi	r23, 32

	ldi	r22, 0
	ldi	r21, 0

	ldi	r20, 0x30

.print_reg_loop:
	/* Loop through register values */

	ldi	r24, 'R'
	call	serial_tx_byte

	mov	r24, r22
	add	r24, r20
	call	serial_tx_byte

	mov	r24, r21
	add	r24, r20
	call	serial_tx_byte

	ldi	r24, ':'
	call	serial_tx_byte

	ldi	r24, '\t'
	call	serial_tx_byte

	pop	r24
	call	_print_byte_hex

	call	_print_nl

	inc	r21
	cpi	r21, 10
	brlt	.prl0
	ldi	r21, 0
	inc	r22
.prl0:

	dec	r23
	brne	.print_reg_loop


	/* PC from stack */
	call	avr_mcu_pc_size_bytes
	mov	r19, r24
	cpi	r19, 3
	brlt	.pc0
	pop	r25
.pc0:
	pop	r23
	pop	r22

	/* SP at call site */
	in	r21, 0x3e
	in	r20, 0x3d


	ldi	r24, 'P'
	call	serial_tx_byte
	ldi	r24, 'C'
	call	serial_tx_byte
	ldi	r24, ':'
	call	serial_tx_byte
	ldi	r24, '\t'
	call	serial_tx_byte
	cpi	r19, 3
	brlt	.pc1
	mov	r24, r25
	call	_print_byte_hex
.pc1:
	mov	r24, r23
	call	_print_byte_hex
	mov	r24, r22
	call	_print_byte_hex

	call	_print_nl

	ldi	r24, 'S'
	call	serial_tx_byte
	ldi	r24, 'P'
	call	serial_tx_byte
	ldi	r24, ':'
	call	serial_tx_byte
	ldi	r24, '\t'
	call	serial_tx_byte
	mov	r24, r21
	call	_print_byte_hex
	mov	r24, r20
	call	_print_byte_hex

	call	_print_nl


	/* Print some stack bytes */
	ldi	r23, 32

.print_stack_bytes_loop:
	ldi	r24, ' '
	call	serial_tx_byte

	pop	r24
	call	_print_byte_hex

	dec	r23
	mov	r22, r23
	andi	r22, 0x03
	brne	.psbl0
	call	_print_nl
.psbl0:
	cpi	r23, 0
	brne	.print_stack_bytes_loop


.ds_sleep:
	sleep
	rjmp	.ds_sleep


_print_nl:
	push	r24
	ldi	r24, 0x0d
	call	serial_tx_byte
	ldi	r24, 0x0a
	call	serial_tx_byte
	pop	r24
	ret


_print_byte_hex:
	push	r21
	push	r20

	ldi	r21, 0x30
	ldi	r20, 0x27

	push	r24
	swap	r24
	andi	r24, 0x0f
	cpi	r24, 0x0a
	brlt	.pbh0
	add	r24, r20
.pbh0:
	add	r24, r21
	call	serial_tx_byte
	pop	r24

	push	r24
	andi	r24, 0x0f
	cpi	r24, 0x0a
	brlt	.pbh1
	add	r24, r20
.pbh1:
	add	r24, r21
	call	serial_tx_byte
	pop	r24

	pop	r20
	pop	r21

	ret
