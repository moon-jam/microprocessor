List p=18f4520 
    #include <pic18f4520.inc>
    CONFIG OSC = INTIO67
    CONFIG WDT = OFF
    org 0x00

    test1:
        ; MOVLW 0b10111011
        ; MOVWF 0x00
    
    test2:
        MOVLW 0b01111100
        MOVWF 0x00

    MOVF 0x00, 0
    MOVWF 0x02      ;in
    CLRF 0x10       ;out
    CLRF 0x03       ;i=0
    CLRF 0x04       ;j=0
    
    ; for (i=0; i<8; ++i)
    ;   j |= 1;
    ;   if (j&in == j) j<<1, out++;
    ;   else in>>1;
    loop:
        ;   j |= 1;
        MOVLW 0x01
        IORWF 0x04
        
        ;   if (j&in == j) j<<1, out++;
        MOVF 0x02, 0
        ANDWF 0x04, 0
        CPFSEQ 0x04
            GOTO otherwise
            RLNCF 0x04
            INCF 0x10
            GOTO end_if
        
        ;   else in>>1;
        otherwise:
            RRNCF 0x02, 1

        end_if:
        ; for (i=0; i<8; ++i)
        INCF 0x03
        MOVLW 0x08
        CPFSLT 0x03             ;[0x03] < 8
            GOTO finish
        GOTO loop

    finish:
        NOP
    end