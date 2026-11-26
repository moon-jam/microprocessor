#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"
#include <stdio.h>

#define BUTTON_PIN PIN_RB0
#define LED_PIN    PIN_RD0
#define BLINK_PIN  PIN_RA1
#define BLINK_TIMER TIMER3

static volatile bool pressed;
static volatile bool has_byte;
static volatile uint8_t received;

void __interrupt(high_priority) isrHigh(void) {
    if (pinInterruptFired(BUTTON_PIN)) pressed = true;
}

void __interrupt(low_priority) isrLow(void) {
    uint8_t byte;
    if (timerInterruptFired(BLINK_TIMER)) digitalToggle(BLINK_PIN);
    if (uartRead(&byte)) {
        received = byte;
        has_byte = true;
    }
}

void main(void) {
    uint8_t presses = 0;

    halBegin();
    pinMode(LED_PIN, OUTPUT);
    pinMode(BLINK_PIN, OUTPUT);
    uartBegin(9600);
    uartPrint("ready\r\n");

    if (!timerAttachInterrupt(BLINK_TIMER, 500, PRIORITY_LOW)) {
        uartPrint("Blink timer setup failed\r\n");
    }
    pinAttachInterrupt(BUTTON_PIN, FALLING, PRIORITY_HIGH);
    uartAttachInterrupt(PRIORITY_LOW);

    while (1) {
        if (pressed) {
            delayMs(20);
            pressed = false;
            if (digitalRead(BUTTON_PIN) == LOW) {
                digitalToggle(LED_PIN);
                printf("presses: %u\r\n", ++presses);
            }
        }

        if (has_byte) {
            has_byte = false;
            uartWrite(received);
        }
    }
}
