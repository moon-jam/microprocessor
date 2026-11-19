/* 
 * A template for lib/pic18f4520_hal.h
 * The source file should change include path if its path is not xx/xx/xx.c.
 */
#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"

#define POT_PIN PIN_RA0
#define BUTTON_PIN PIN_RB0
#define SERVO_PIN  PIN_RC2

static const Pin leds[] = {PIN_RA1, PIN_RA2, PIN_RA3};

void main(void) {
    uint16_t value;
    uint8_t region = 0;
    Button button;
    Servo servo;

    halBegin();
    analogBegin(POT_PIN);
    buttonBegin(&button, BUTTON_PIN, false, 20); // Active Low

    servoAttach(&servo, SERVO_PIN, 0, 180, 500, 2400);
    servoWrite(&servo, 0);

    for (uint8_t i = 0; i < 3; i++)
        pinMode(leds[i], OUTPUT);

    while (1) {
        if (analogRead(POT_PIN, &value)) {
            region = (uint32_t)value * 3 / 1024;

            for (uint8_t i = 0; i < 3; i++)
                digitalWrite(leds[i], i == region ? HIGH : LOW);
        }

        if (buttonPressed(&button))
            servoWrite(&servo, region * 90);
    }
}