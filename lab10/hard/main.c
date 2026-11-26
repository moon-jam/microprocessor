#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool readKey(uint8_t *key) {
    static bool skip_lf;

    if (!uartRead(key)) return false;
    if (*key == '\n' && skip_lf) {
        skip_lf = false;
        return false;
    }
    skip_lf = *key == '\r';
    if (skip_lf) *key = '\n';
    return true;
}

static void readLine(char *line, uint8_t size) {
    uint8_t used = 0;
    bool invalid = false;

    for (;;) {
        uint8_t key;
        if (!readKey(&key)) continue;
        if (key == '\n') {
            line[invalid ? 0 : used] = '\0';
            uartPrint("\r\n");
            return;
        }
        if (key == '\0' || used == size - 1U) invalid = true;
        if (!invalid) {
            line[used++] = (char)key;
            uartWrite(key);
        }
    }
}

static int16_t readPotAngle(void) {
    int16_t shown = 91;

    for (;;) {
        uint8_t key;
        if (shown != 91 && readKey(&key) && key == '\n') {
            uartPrint("\r\n");
            return shown;
        }

        uint16_t raw;
        if (analogRead(PIN_RA0, &raw)) {
            int16_t angle = (int16_t)(((uint32_t)raw * 180U + 511U) / 1023U) - 90;
            if (angle != shown) {
                if (shown != 91) uartPrint("\b\b\b\b");
                printf("%4d", (int)angle);
                shown = angle;
            }
        }
    }
}

static void moveTo(Servo *servo, uint8_t position, int16_t target) {
    for (;;) {
        int16_t remaining = target - servo->angle;
        uint16_t started = timerNow();
        printf("Point %u/4 | Current: %d | Target: %d\r\n",
               (unsigned)position, (int)servo->angle, (int)target);
        if (!remaining) {
            uint16_t elapsed = (uint16_t)(timerNow() - started);
            uint16_t elapsed_ms = (uint16_t)((uint32_t)elapsed * HAL_TICK_US / 1000U);
            if (elapsed_ms < 2000U) delayMs(2000U - elapsed_ms);
            return;
        }
        int16_t step = remaining > 5 ? 5 : remaining < -5 ? -5 : remaining;
        servoWrite(servo, servo->angle + step);
        delayMs(20);
    }
}

void main(void) {
    Servo servo;
    char line[16];
    int16_t targets[4];

    halBegin();
    if (!uartBegin(9600) || !analogBegin(PIN_RA0) ||
        !servoAttach(&servo, PIN_RC2, -90, 90, 500, 2400) || !servoWrite(&servo, 0)) {
        for (;;) { NOP(); }
    }

    for (;;) {
        uartPrint("Select Mode (m1/m2)\r\n");
        readLine(line, sizeof(line));
        bool pot = !strcmp(line, "m1");
        if (!pot && strcmp(line, "m2")) continue;

        for (uint8_t i = 0; i < 4U; ++i) {
            if (pot) {
                printf("Point %u/4: ", (unsigned)(i + 1U));
                targets[i] = readPotAngle();
            } else {
                for (;;) {
                    printf("Point %u/4 (-90..90): ", (unsigned)(i + 1U));
                    readLine(line, sizeof(line));
                    char *end;
                    long angle = strtol(line, &end, 10);
                    if (end != line && !*end && angle >= -90 && angle <= 90) {
                        targets[i] = (int16_t)angle;
                        break;
                    }
                }
            }
        }

        for (uint8_t i = 0; i < 4U; ++i) moveTo(&servo, i + 1U, targets[i]);
        uartPrint("DONE\r\n");
    }
}
