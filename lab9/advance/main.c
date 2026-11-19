/* 
 * A template for lib/pic18f4520_hal.h
 * The source file should change include path if its path is not xx/xx/xx.c.
 */
#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"

#define POT_PIN PIN_RA0

static const Pin leds[] = {PIN_RA1, PIN_RA2, PIN_RA3, PIN_RA4};

void main(void) {
    uint16_t value, previous = 0;
    bool descending = false;

    halBegin();
    analogBegin(POT_PIN);

    for (uint8_t i = 0; i < 4; i++)
        pinMode(leds[i], OUTPUT);

    while (1) {
        if (!analogRead(POT_PIN, &value))
            continue;

        // Detect rotation direction (8-count noise tolerance)
        if (value > previous + 8) {
            descending = false;
            previous = value;
        } else if (value + 8 < previous) {
            descending = true;
            previous = value;
        }

        uint8_t number = descending
            ? 3 * ((uint32_t)value * 6 / 1024)
            : (uint32_t)value * 16 / 1024;

        digitalWriteBits(leds, 4, number);
    }
}