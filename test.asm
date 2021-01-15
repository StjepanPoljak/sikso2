; test program

_start:
	LDA #$66
	ASL
	STA $0e
	LDA #$11
	ADC $0e
_end:	NOP
	JMP _start
