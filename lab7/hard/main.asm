
PROCESSOR 18F4520
#include <xc.inc>

CONFIG OSC = INTIO67
CONFIG WDT = OFF
CONFIG LVP = OFF
CONFIG PBADEN = OFF
CONFIG XINST = OFF
CONFIG MCLRE = ON

; MODE: 0=idle, 1=playing, 2=finished
; KEY:  0=ready, 1=debouncing, 2=held
MODE     EQU 0x20
ACTIVE   EQU 0x21         ; Moving LED
LOCKS    EQU 0x22         ; Locked LEDs
PERIOD   EQU 0x23         ; 10-ms ticks per movement
TICKS    EQU 0x24
KEY      EQU 0x25
DB_CNT   EQU 0x26
HOLD_L   EQU 0x27
HOLD_H   EQU 0x28
SHOT     EQU 0x29         ; LED position at press edge

PSECT mainCode,class=CODE,reloc=2,abs,ovrld

ORG 0x0000
    GOTO Init

ORG 0x0008
    GOTO ISR

; ISR:
; INT0 -> record press, disable INT0
; Timer2 -> handle button and game timing
ISR:
    BTFSS INT0IE
    BRA CheckTimer
    BTFSC INT0IF
    CALL PressEdge

CheckTimer:
    BTFSS TMR2IF
    BRA IsrExit
    BCF TMR2IF
    CALL ButtonTick
    CALL GameTick

IsrExit:
    RETFIE 1

PressEdge:
    BCF INT0IE            ; Ignore further edges
    BCF INT0IF
    MOVLW 1
    MOVWF KEY
    MOVFF ACTIVE, SHOT
    MOVLW 3               ; At least 20 ms debounce
    MOVWF DB_CNT
    CLRF HOLD_L
    CLRF HOLD_H
    RETURN


; ButtonTick (every 10 ms):
; Debounce -> lock/unlock immediately
; Held 3 seconds -> reset
; Released -> re-enable INT0
ButtonTick:
    MOVF KEY, W
    BZ ButtonEnd
    XORLW 1
    BZ PressCheck
    BRA Held

PressCheck:
    BTFSC PORTB, 0
    BRA ReArm
    DECFSZ DB_CNT, F
    RETURN

    MOVLW 2
    MOVWF KEY

    MOVF MODE, W
    BZ StartGame
    XORLW 1
    BNZ ButtonEnd

    CALL ToggleLock
    RETURN

StartGame:
    MOVLW 1
    MOVWF MODE
    MOVLW 0x10
    MOVWF ACTIVE
    MOVLW 100
    MOVWF PERIOD
    CLRF TICKS
    CALL Show
    RETURN

Held:
    BTFSC PORTB, 0
    BRA ReArm             ; No release debounce

    MOVF MODE, W
    XORLW 1
    BNZ ButtonEnd

    ; Long press: 300 x 10 ms = 3 seconds
    INCF HOLD_L, F
    BTFSC STATUS, 2       ; Z flag: low byte overflow
    INCF HOLD_H, F

    MOVF HOLD_H, W
    XORLW 1
    BNZ ButtonEnd
    MOVLW 44              ; 300 = 0x012C
    CPFSLT HOLD_L
    CALL ResetGame
    RETURN

ReArm:
    CLRF KEY
    BCF INT0IF
    BSF INT0IE

ButtonEnd:
    RETURN


; GameTick:
; Playing -> move after PERIOD ticks
; Finished -> reset after 200 ticks
GameTick:
    MOVF MODE, W
    XORLW 1
    BZ Running

    MOVF MODE, W
    XORLW 2
    BNZ GameEnd

    INCF TICKS, F
    MOVLW 200
    CPFSLT TICKS
    CALL ResetGame

GameEnd:
    RETURN

Running:
    INCF TICKS, F
    MOVF PERIOD, W
    CPFSLT TICKS
    BRA StepLED
    RETURN

StepLED:
    CLRF TICKS
    RLNCF ACTIVE, F
    BTFSC ACTIVE, 0       ; Wrap 0x80 -> 0x10
    BRA WrapLED
    BRA Show

WrapLED:
    MOVLW 0x10
    MOVWF ACTIVE
    BRA Show


; ToggleLock:
; Lock:   PERIOD -= 25
; Unlock: PERIOD += 25
ToggleLock:
    MOVF SHOT, W
    ANDWF LOCKS, W
    BNZ UnlockLED

LockLED:
    MOVF SHOT, W
    IORWF LOCKS, F
    MOVLW 25
    SUBWF PERIOD, F

    MOVF LOCKS, W
    XORLW 0xF0
    BZ Won
    BRA LockUpdated

UnlockLED:
    MOVF SHOT, W
    XORWF LOCKS, F
    MOVLW 25
    ADDWF PERIOD, F

LockUpdated:
    CLRF TICKS
    CALL Show
    RETURN

Won:
    MOVLW 2
    MOVWF MODE
    CLRF TICKS
    MOVLW 0xF0
    MOVWF LATD
    RETURN


; LED output = ACTIVE OR LOCKS
Show:
    MOVF ACTIVE, W
    IORWF LOCKS, W
    MOVWF LATD
    RETURN

ResetGame:
    CLRF MODE
    CLRF ACTIVE
    CLRF LOCKS
    CLRF TICKS
    CLRF LATD
    RETURN


; Init:
; RB0 input with pull-up, RD4-RD7 output
; INT0 falling edge, Timer2 every 10 ms
Init:
    MOVLW 0x40
    MOVWF OSCCON          ; Fosc = 1 MHz
    MOVLW 0x0F
    MOVWF ADCON1          ; Digital I/O

    CLRF LATD
    MOVLW 0x0F
    ANDWF TRISD, F        ; RD4-RD7 output

    BSF TRISB, 0          ; RB0 input
    BSF LATB, 0
    BCF RBPU              ; Enable PORTB pull-ups
    BCF INTEDG0           ; INT0 falling edge

    CALL ResetGame
    CLRF KEY

    ; Timer2: 10 ms
    ; Prescale 1:1, Postscale 1:10
    CLRF TMR2
    MOVLW 249
    MOVWF PR2
    MOVLW 0b01001100
    MOVWF T2CON

    BCF IPEN              ; Single interrupt vector
    BCF TMR2IF
    BSF TMR2IE
    BCF INT0IF
    BSF INT0IE
    BSF PEIE
    BSF GIE

Main:
    BRA Main

END
