LIST p=18f4520
#include <pic18f4520.inc>

    CONFIG OSC = INTIO67 ; Set internal oscillator to 1 MHz
    CONFIG WDT = OFF     ; Disable Watchdog Timer
    CONFIG LVP = OFF     ; Disable Low Voltage Programming

    L1 EQU 0x14         ; Define L1 memory location
    L2 EQU 0x15         ; Define L2 memory location
	CNT EQU 0x16
	STATE EQU 0x17
	LED EQU 0x18
    org 0x00            ; Set program start address to 0x00

; instruction frequency = 1 MHz / 4 = 0.25 MHz
; instruction time = 1/0.25 = 4 μs
; Total_cycles = 2 + (2 + 8 * num1 + 3) * num2 cycles
; num1 = 111, num2 = 70, Total_cycles = 62512 cycles
; Total_delay ~= Total_cycles * instruction time = 0.25 s
DELAY macro num1, num2
    local LOOP1         ; Inner loop
    local LOOP2         ; Outer loop
    
    ; 2 cycles
    MOVLW num2          ; Load num2 into WREG
    MOVWF L2            ; Store WREG value into L2
    
    ; Total_cycles for LOOP2 = 2 cycles
    LOOP2:
    MOVLW num1          
    MOVWF L1  
    
    ; Total_cycles for LOOP1 = 8 cycles
    LOOP1:
    NOP                 ; busy waiting
    NOP
    NOP
    NOP
    NOP
    DECFSZ L1, 1        
    BRA LOOP1           ; BRA instruction spends 2 cycles
    
    ; 3 cycles
    DECFSZ L2, 1        ; Decrement L2, skip if zero
    BRA LOOP2           
endm

start:
int:
; let pin can receive digital signal
    MOVLW 0x0f          ; Set ADCON1 register for digital mode
    MOVWF ADCON1        ; Store WREG value into ADCON1 register
    CLRF PORTB          ; Clear PORTB
    BSF TRISB, 0        ; Set RB0 as input (TRISB = 0000 0001)
    CLRF LATA           ; Clear LATA
    BCF TRISA, 0        ; Set RA0 as output (TRISA = 0000 0000)
    BCF TRISA, 1        ; Set RA1 as output (TRISA = 0000 0000)
    BCF TRISA, 2        ; Set RA2 as output (TRISA = 0000 0000)
	MOVLW 15
	MOVWF CNT
	CLRF STATE
	CLRF LED
	BSF	STATE, 1
stay_init:
    BTFSC PORTB, 0      ; Check if PORTB bit 0 is low (button pressed)
    BRA stay_init
    
; Button check
check_process:          
    DELAY 102, 10 		; Call delay macro to delay for about 0.033 seconds
    BTFSC PORTB, 0
	BCF	STATE, 1
    BTFSC PORTB, 0      ; Check if PORTB bit 0 is low (button pressed)
    BRA update
	BTFSS STATE, 1
	BTG	STATE, 0
	BSF	STATE, 1

update:
	DECFSZ CNT
    BRA lightup         ; If button is pressed, branch to lightup
	MOVLW 15
	MOVWF CNT
	BTFSC STATE, 0
	DECF LED
	BTFSS STATE, 0
	INCF LED
    
lightup:
	MOVF LED, 0
	ANDLW 7
	MOVWF LATA
    BRA check_process   
end
