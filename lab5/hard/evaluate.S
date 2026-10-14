#include "xc.inc"

GLOBAL _evaluate
EXTRN _evaluate$0
EXTRN _evaluate$1
EXTRN _evaluate$2
EXTRN _evaluate$3
EXTRN _evaluate$4
EXTRN _evaluate$5
EXTRN _evaluate$6

#define op1	_evaluate$1
#define op2	_evaluate$3
#define op3	_evaluate$5
#define _w	_evaluate$0
#define x	_evaluate$2
#define y	_evaluate$4
#define z	_evaluate$6

GLOBAL RHS, LHS, OP, RESULT, TEMP
PSECT udata_acs	; little endian
RHS:	DS 2
LHS:	DS 2
OP:		DS 1
RESULT:	DS 2
TEMP:	DS 2

PSECT code
_evaluate:
	; op1	op2	op3
	; +-	+-	+-	1->2->3
	; *		+-	+-	1->2->3	(1)
	; +-	*	+-	2->1->3	(2)
	; +-	+-	*	3->1->2	(3)
	; *		*	+-	1->2->3	(1)
	; +-	*	*	2->3->1	(4)
	; *		+-	*	3->1->2	(3)
	; *		*	*	x
	; (3): op2 not * and op3 is *
	MOVLW	'*'
	CPFSEQ	op3, a
	GOTO	consider_213	;op3=+-
	CPFSEQ	op2, a
	GOTO	_312
	GOTO	_231	;op3=* & op2=*
consider_213:
	CPFSEQ	op2, a
	GOTO	_123
	CPFSEQ	op1, a
	GOTO	_213
	GOTO	_123

_123:
	MOVFF	_w, LHS
	MOVFF	_w+1, LHS+1
	MOVFF	x, RHS
	MOVFF	x+1, RHS+1
	MOVFF	op1, OP
	CALL	CAL

	MOVFF	RESULT, LHS
	MOVFF	RESULT+1, LHS+1
	MOVFF	y, RHS
	MOVFF	y+1, RHS+1
	MOVFF	op2, OP
	CALL	CAL

	MOVFF	RESULT, LHS
	MOVFF	RESULT+1, LHS+1
	MOVFF	z, RHS
	MOVFF	z+1, RHS+1
	MOVFF	op3, OP
	CALL	CAL

	GOTO	store_result

_231:
	MOVFF	x, LHS
	MOVFF	x+1, LHS+1
	MOVFF	y, RHS
	MOVFF	y+1, RHS+1
	MOVFF	op2, OP
	CALL	CAL

	MOVFF	RESULT, LHS
	MOVFF	RESULT+1, LHS+1
	MOVFF	z, RHS
	MOVFF	z+1, RHS+1
	MOVFF	op3, OP
	CALL	CAL

	MOVFF	_w, LHS
	MOVFF	_w+1, LHS+1
	MOVFF	RESULT, RHS
	MOVFF	RESULT+1, RHS+1
	MOVFF	op1, OP
	CALL	CAL

	GOTO	store_result

_213:
	MOVFF	x, LHS
	MOVFF	x+1, LHS+1
	MOVFF	y, RHS
	MOVFF	y+1, RHS+1
	MOVFF	op2, OP
	CALL	CAL

	MOVFF	_w, LHS
	MOVFF	_w+1, LHS+1
	MOVFF	RESULT, RHS
	MOVFF	RESULT+1, RHS+1
	MOVFF	op1, OP
	CALL	CAL

	MOVFF	RESULT, LHS
	MOVFF	RESULT+1, LHS+1
	MOVFF	z, RHS
	MOVFF	z+1, RHS+1
	MOVFF	op3, OP
	CALL	CAL

	GOTO	store_result

_312:
	MOVFF	y, LHS
	MOVFF	y+1, LHS+1
	MOVFF	z, RHS
	MOVFF	z+1, RHS+1
	MOVFF	op3, OP
	CALL	CAL

	MOVFF	RESULT, TEMP
	MOVFF	RESULT+1, TEMP+1

	MOVFF	_w, LHS
	MOVFF	_w+1, LHS+1
	MOVFF	x, RHS
	MOVFF	x+1, RHS+1
	MOVFF	op1, OP
	CALL	CAL

	MOVFF	RESULT, LHS
	MOVFF	RESULT+1, LHS+1
	MOVFF	TEMP, RHS
	MOVFF	TEMP+1, RHS+1
	MOVFF	op2, OP
	CALL	CAL

	GOTO	store_result

; *: 42, +: 43, -: 45
CAL:
	MOVLW	'-'
	CPFSLT	OP, a
	GOTO 	SUB
	MOVLW	'+'
	CPFSLT	OP,	a
	GOTO 	ADD
	GOTO 	MUL

ADD:
	MOVF	LHS, w, a
	ADDWF	RHS, w, a
	MOVWF 	RESULT, a
	MOVF	LHS+1, w, a
	ADDWFC	RHS+1, w, a
	MOVWF 	RESULT+1, a
	return

SUB:
	MOVF	RHS, w, a
	SUBWF	LHS, w, a
	MOVWF 	RESULT, a
	MOVF	RHS+1, w, a
	SUBWFB	LHS+1, w, a
	MOVWF 	RESULT+1, a
	return

MUL:	; guarantee result < 65535
	MOVF	LHS, w, a
	MULWF	RHS, a
	MOVFF	PRODL, RESULT
	MOVFF	PRODH, RESULT+1

	MOVF	LHS+1, w, a
	MULWF	RHS, a
	MOVF	PRODL, w, a
	ADDWF	RESULT+1, f, a

	MOVF	LHS, w, a
	MULWF	RHS+1, a
	MOVF	PRODL, w, a
	ADDWF	RESULT+1, f, a
	return

store_result:
	MOVFF RESULT, _evaluate$0
	MOVFF RESULT+1, _evaluate$0+1
	RETURN
