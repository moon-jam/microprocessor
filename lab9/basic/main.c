#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"

#define POT_PIN PIN_RA0

static const uint8_t digits[] = {1, 4, 1, 3, 6, 0, 3, 7};
static const Pin leds[] = {PIN_RA1, PIN_RA2, PIN_RA3, PIN_RA4};

void main(void) {
    uint16_t value;

    halBegin();
    analogBegin(POT_PIN);

    for (uint8_t i = 0; i < 4; i++)
        pinMode(leds[i], OUTPUT);

    while (1) {
        if (analogRead(POT_PIN, &value)) {
            uint8_t index = (uint32_t)value * sizeof(digits) / 1024;
            digitalWriteBits(leds, 4, digits[index]);
        }
    }
}