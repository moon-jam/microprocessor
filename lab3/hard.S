List p=18f4520 
    #include <pic18f4520.inc>
    CONFIG OSC = INTIO67
    CONFIG WDT = OFF
    org 0x00

    GOTO    start

start:
;---------------------------------------------------------------------
;       Instruction Start
;---------------------------------------------------------------------
        MOVLW   0x04
        MOVWF   0x00, A
        MOVLW   0x05
        MOVWF   0x01, A
        MOVLW   0x03
        MOVWF   0x02, A
        MOVLW   0x01
        MOVWF   0x03, A
        MOVLW   0x02
        MOVWF   0x04, A
        MOVLW   0x00
        MOVWF   0x05, A
;---------------------------------------------------------------------
;       Instruction End
;---------------------------------------------------------------------

    MOVLB   1
    MOVLW   0x00
    MOVWF   0x00,       B   			; PC
    ; while (PC < 0xFE)
loop:
    MOVLW   0xFE
    CPFSLT  0x00,       B
    GOTO    finish
    CLRF    FSR0H
    MOVFF   0x100,      FSR0L 			; FSR0 = PC
	MOVLW 	0x03
	ADDWF	0x00,		F,		B
    MOVFF   POSTINC0,   FSR1
	MOVF	INDF1,		W
	MOVFF	POSTINC0,	FSR1
    SUBWF   INDF1,   	F
	BN		jump
	BZ		jump
	GOTO	loop
jump:
	MOVFF	INDF0,		0x100
	GOTO 	loop

finish:
    NOP
    end