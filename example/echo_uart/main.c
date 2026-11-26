#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"

void main(void) {
    uint8_t line[100];
    uint8_t used = 0;
    bool overflow = false;

    halBegin();
    uartBegin(9600);
    pinMode(PIN_RA1, OUTPUT);
    digitalWrite(PIN_RA1, HIGH);

    while (1) {
        uint8_t byte;
        if (!uartRead(&byte)) continue;

        if (byte == '\n' || byte == '\r') {
            if (overflow) {
                uartPrint("Line too long\r\n");
            } else {
                for (uint8_t i = 0; i < used; ++i) uartWrite(line[i]);
                uartPrint("\r\n");
            }
            used = 0;
            overflow = false;
        } else if (used < sizeof(line)) {
            line[used++] = byte;
        } else {
            overflow = true;
        }
    }
}
