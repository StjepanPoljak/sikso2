; test program

_start:
	LDA #$66
	STA $0e
	LDA #$11
	ADC $0e
_end:	NOP ; test label
	JMP _end
