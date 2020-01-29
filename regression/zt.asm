	include	mmi.avh
	include	debio.avh
	;menu
	extern	p083a(data)
	extern	p083b(data)
	extern	t14(xdata)
	;zt
	extern	zeroTensiometerState(xdata)
	extern	oneTimezeroTensiometer(data)
	;reuse
	extern	linkerspaak(bit)
	extern	dataokb46(bit)


state2curpos:
	mov	dptr, #zeroTensiometerState
	movx	a, @dptr
	call	pick_side
	ret

	db	'=>', 0
printArrow:
	public	printArrow
	call	state2curpos
	inc	a
	mov	r7, a
	mov	r6, #2
	call	setposa
	mov	dptr, #printarrow - 3
	jmp	nih_print
clearArrow:
	call	state2curpos
	inc	a
	mov	r7, a
	mov	r6, #2
	call	setposa
	mov	a, #' '
	call	chrout
	jmp	chrout
