#include "xc.inc"
GLOBAL _gcd
PSECT mytext, local, class=CODE, reloc=2
     
; binary GCD
; gcd(0, b) = b
; gcd(2a, 2b) = 2*gcd(a, b)
; gcd(2a, b) = gcd(a, b)
; gcd(a, b) = gcd(a, b-a) b>a
_gcd:
	MOVWF 	0x011
	MOVFF 	0x001, 0x013
	CLRF    0x20		; k: 2^k is the common divisor


gcd_start:
	MOVF    0x11, W
	BNZ     check_zb
	MOVFF   0x013, 0x001
	GOTO 	put_back
check_zb:
	MOVF    0x13, W
	BNZ     even_check
	MOVFF   0x011, 0x001
	GOTO 	put_back


even_check:
	BTFSC	0x11, 0
	GOTO	odd_a
	RRNCF   0x11
	GOTO	even_a
odd_a:
	BTFSC	0x13, 0
	GOTO	both_odd
	RRNCF   0x13
	GOTO    gcd_start
even_a:
	BTFSC	0x13, 0
	GOTO    gcd_start	;even_a and odd_b
	RRNCF   0x13
	INCF    0x20
	GOTO    gcd_start	;both even


both_odd:
	MOVF    0x13, W
	SUBWF   0x11, W		;aL-bL < 0
	BNC     bigger_b
bigger_a:
	MOVF    0x13, W
	SUBWF   0x11, F
	GOTO 	gcd_start
bigger_b:
	MOVF    0x11, W
	SUBWF   0x13, F
	GOTO 	gcd_start

put_back:
	MOVF    0x20, W
	BZ		finish
	RLNCF   0x01
	DECF    0x20
	GOTO	put_back

finish:
	MOVF    0x01, W
	RETURN
