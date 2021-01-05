; test program

_start:
	LDA #$66
	ASL A
	STA $0e
	LDA #$11
	ADC $0e
_end:	NOP ; test label
	;JMP _end
