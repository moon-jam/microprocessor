#include "xc.inc"

GLOBAL _count_primes
EXTRN _count_primes$0
EXTRN _count_primes$1

PSECT udata_acs
xL:    DS 1
xH:    DS 1
loL:   DS 1
loH:   DS 1
hiL:   DS 1
hiH:   DS 1
mdL:   DS 1
mdH:   DS 1
pL:    DS 1
ansL:  DS 1
ansH:  DS 1

PSECT code
_count_primes:
    ; L = lower_bound(n)
    MOVFF   _count_primes$0, xL
    MOVFF   _count_primes$0+1, xH
    CALL    lower_bound
    MOVFF   loL, ansL
    MOVFF   loH, ansH

    ; R = lower_bound(m+1)
    MOVFF   _count_primes$1, xL
    MOVFF   _count_primes$1+1, xH

    ; Special case: m = 65535
    MOVLW   0xFF
    CPFSEQ  xL, a
    GOTO    normal_m
    CPFSEQ  xH, a
    GOTO    normal_m

    MOVLW   LOW(6542)
    MOVWF   loL, a
    MOVLW   HIGH(6542)
    MOVWF   loH, a
    GOTO    result

normal_m:
    MOVLW   1
    ADDWF   xL, f, a
    MOVLW   0
    ADDWFC  xH, f, a
    CALL    lower_bound

result:
    ; ans = R - L
    MOVF    ansL, W, a
    SUBWF   loL, f, a
    MOVF    ansH, W, a
    SUBWFB  loH, f, a

    MOVFF   loL, _count_primes$0
    MOVFF   loH, _count_primes$0+1
    RETURN


; Input: xH:xL
; Output: loH:loL = lower_bound(x)
lower_bound:
    CLRF    loL, a
    CLRF    loH, a
    MOVLW   LOW(6542)
    MOVWF   hiL, a
    MOVLW   HIGH(6542)
    MOVWF   hiH, a

loop:
    ; while (lo != hi)
    MOVF    loL, W, a
    XORWF   hiL, W, a
    BNZ     search
    MOVF    loH, W, a
    XORWF   hiH, W, a
    BZ      done

search:
    ; mid = (lo + hi) / 2
    MOVF    loL, W, a
    ADDWF   hiL, W, a
    MOVWF   mdL, a
    MOVF    loH, W, a
    ADDWFC  hiH, W, a
    MOVWF   mdH, a
    BCF     STATUS, 0, a
    RRCF    mdH, f, a
    RRCF    mdL, f, a

    ; TBLPTR = prime_table + 2 * mid
    MOVF    mdL, W, a
    ADDWF   mdL, W, a
    MOVWF   TBLPTRL, a
    MOVF    mdH, W, a
    ADDWFC  mdH, W, a
    MOVWF   TBLPTRH, a

    MOVLW   LOW(prime_table)
    ADDWF   TBLPTRL, f, a
    MOVLW   HIGH(prime_table)
    ADDWFC  TBLPTRH, f, a
    CLRF    TBLPTRU, a

    ; Read 16-bit prime
    TBLRD*+
    MOVFF   TABLAT, pL
    TBLRD*

    ; if (prime < x) go right
    MOVF    xH, W, a
    SUBWF   TABLAT, W, a
    BNC     right
    BNZ     left
    MOVF    xL, W, a
    SUBWF   pL, W, a
    BNC     right

left:
    ; hi = mid
    MOVFF   mdL, hiL
    MOVFF   mdH, hiH
    GOTO    loop

right:
    ; lo = mid + 1
    INCF    mdL, f, a
    BTFSC   STATUS, 2, a
    INCF    mdH, f, a
    MOVFF   mdL, loL
    MOVFF   mdH, loH
    GOTO    loop

done:
    RETURN

#include "prime_table.inc"