List p=18f4520
#include <pic18f4520.inc>
CONFIG OSC = INTIO67
CONFIG WDT = OFF
org 0x00

SET8 MACRO addr, val
    MOVLW val
    MOVWF addr
ENDM

MOV16 MACRO sh, sl, dh, dl
    MOVFF sh,dh
    MOVFF sl,dl
ENDM

; load 8-bit numerator/denominator as 16-bit fraction
LOADF MACRO n, d, nh, nl, dh, dl
    CLRF  nh
    MOVFF n,nl
    CLRF  dh
    MOVFF d,dl
ENDM

MOVFRAC MACRO snh, snl, sdh, sdl, dnh, dnl, ddh, ddl
    MOVFF snh,dnh
    MOVFF snl,dnl
    MOVFF sdh,ddh
    MOVFF sdl,ddl
ENDM

; 16-bit multiply
; ah:al * bh:bl -> rh:rl
; high*high is unnecessary because all required values fit in 16 bits
MUL16 MACRO ah, al, bh, bl, rh, rl
    ; low * low
    MOVF  al,W
    MULWF bl
    MOVFF PRODH,rh
    MOVFF PRODL,rl

    ; high1 * low2
    MOVF  ah,W
    MULWF bl
    MOVF  PRODL,W
    ADDWF rh,F

    ; low1 * high2
    MOVF  al,W
    MULWF bh
    MOVF  PRODL,W
    ADDWF rh,F
ENDM

goto test2

test1:
    SET8 0x00,0xC7
    SET8 0x01,0xE9
    SET8 0x02,0x11
    SET8 0x03,0x05
    SET8 0x04,0x03
    SET8 0x05,0x28
    SET8 0x06,0x00
    SET8 0x07,0x02
    goto start

test2:
    SET8 0x00,0x71
    SET8 0x01,0x1B
    SET8 0x02,0xB4
    SET8 0x03,0xC4
    SET8 0x04,0xF3
    SET8 0x05,0x31
    SET8 0x06,0x01
    SET8 0x07,0x03
    goto start


; ============================================================
; 40:41 / 42:43
; quotient  -> 40:41
; remainder -> 46:47
; ============================================================

div16:
    CLRF 0x46
    CLRF 0x47
    SET8 0x4A,0x10

div_loop:
    ; shift remainder:quotient left
    BCF   STATUS,0
    RLCF  0x41,F
    RLCF  0x40,F
    RLCF  0x47,F
    RLCF  0x46,F

    ; remainder >= divisor ?
    MOVF  0x42,W
    SUBWF 0x46,W
    BNC   div_next
    BNZ   div_sub

    MOVF  0x43,W
    SUBWF 0x47,W
    BNC   div_next

div_sub:
    MOVF   0x43,W
    SUBWF  0x47,F
    MOVF   0x42,W
    SUBWFB 0x46,F
    BSF    0x41,0

div_next:
    DECFSZ 0x4A,F
    goto   div_loop
    RETURN


; ============================================================
; GCD - Euclidean algorithm
; 30:31, 32:33 -> 34:35
; ============================================================

GCD:
gcd_loop:
    ; v == 0 ?
    MOVF  0x32,W
    IORWF 0x33,W
    BZ    gcd_done

    ; u % v
    MOV16 0x30,0x31,0x40,0x41
    MOV16 0x32,0x33,0x42,0x43
    RCALL div16

    ; (u,v) = (v,remainder)
    MOV16 0x32,0x33,0x30,0x31
    MOV16 0x46,0x47,0x32,0x33
    goto  gcd_loop

gcd_done:
    MOV16 0x30,0x31,0x34,0x35
    RETURN


; ============================================================
; fractionCalc
;
; (20:21)/(22:23) 24 (25:26)/(27:28) = (29:2A)/(2B:2C)
;
; 0:+  1:-  2:*  3:/
; ============================================================

fractionCalc:
    MOVF 0x24,W
    BZ   fc_addsub

    XORLW 0x01
    BZ    fc_addsub

    MOVF  0x24,W
    XORLW 0x02
    BZ    fc_mul

    goto fc_div

; a/b +/- c/d
fc_addsub:
    ; ad and cb
    MUL16 0x20,0x21,0x27,0x28,0x29,0x2A
    MUL16 0x25,0x26,0x22,0x23,0x48,0x49

    MOVF 0x24,W
    BZ   fc_add

    ; ad - cb
    MOVF   0x49,W
    SUBWF  0x2A,F
    MOVF   0x48,W
    SUBWFB 0x29,F
    goto   fc_den

fc_add:
    ; ad + cb
    MOVF   0x49,W
    ADDWF  0x2A,F
    MOVF   0x48,W
    ADDWFC 0x29,F

fc_den:
    ; bd
    MUL16 0x22,0x23,0x27,0x28,0x2B,0x2C
    goto fc_reduce

; a/b * c/d
fc_mul:
    MUL16 0x20,0x21,0x25,0x26,0x29,0x2A
    MUL16 0x22,0x23,0x27,0x28,0x2B,0x2C
    goto fc_reduce


; a/b / c/d = ad / bc
fc_div:
    MUL16 0x20,0x21,0x27,0x28,0x29,0x2A
    MUL16 0x22,0x23,0x25,0x26,0x2B,0x2C

; reduce by GCD
fc_reduce:
    MOV16 0x29,0x2A,0x30,0x31
    MOV16 0x2B,0x2C,0x32,0x33
    RCALL GCD

    ; numerator / gcd
    MOV16 0x29,0x2A,0x40,0x41
    MOV16 0x34,0x35,0x42,0x43
    RCALL div16
    MOV16 0x40,0x41,0x29,0x2A

    ; denominator / gcd
    MOV16 0x2B,0x2C,0x40,0x41
    MOV16 0x34,0x35,0x42,0x43
    RCALL div16
    MOV16 0x40,0x41,0x2B,0x2C
    RETURN

start:
    BTFSC 0x06,1
    goto  leftFirst

	; right-first only when op1 is +/- and op2 is */
    BTFSC 0x07,1
    goto  rightFirst

    goto leftFirst


; (A/B op1 C/D) op2 E/F
leftFirst:
    LOADF 0x00,0x01,0x20,0x21,0x22,0x23
    LOADF 0x02,0x03,0x25,0x26,0x27,0x28
    MOVFF 0x06,0x24
    RCALL fractionCalc

    MOVFRAC 0x29,0x2A,0x2B,0x2C,0x20,0x21,0x22,0x23
    LOADF   0x04,0x05,0x25,0x26,0x27,0x28
    MOVFF   0x07,0x24
    RCALL   fractionCalc
    goto    storeResult


; A/B op1 (C/D op2 E/F)
rightFirst:
    LOADF 0x02,0x03,0x20,0x21,0x22,0x23
    LOADF 0x04,0x05,0x25,0x26,0x27,0x28
    MOVFF 0x07,0x24
    RCALL fractionCalc

    MOVFRAC 0x29,0x2A,0x2B,0x2C,0x25,0x26,0x27,0x28
    LOADF   0x00,0x01,0x20,0x21,0x22,0x23
    MOVFF   0x06,0x24
    RCALL   fractionCalc


storeResult:
    MOVFRAC 0x29,0x2A,0x2B,0x2C,0x10,0x11,0x12,0x13

finish:
    NOP

END