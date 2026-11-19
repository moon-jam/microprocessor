#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"

#define BUTTON_PIN       PIN_RB0
#define SERVO_PIN        PIN_RC2
#define STEP_DEG         3
#define STEPS_PER_CYCLE  (360 / STEP_DEG)

void main(void) {
    const uint16_t cycle_ms[3] = {12000, 6000, 3000};
    Button button;
    Servo servo;
    uint8_t speed = 0;
    int16_t angle = -90;
    int8_t direction = 1;
    uint16_t last, interval;

    halBegin();
    buttonBegin(&button, BUTTON_PIN, false, 20); // Active Low
    servoAttach(&servo, SERVO_PIN, -90, 90, 500, 2400);

    interval = timerTicks(cycle_ms[speed], STEPS_PER_CYCLE);
    last = timerNow();

	servoWrite(&servo, angle);
    while (1) {
        if (buttonReleased(&button)) {
            speed = (speed + 1) % 3;
            interval = timerTicks(cycle_ms[speed], STEPS_PER_CYCLE);
            last = timerNow();
        }

        if ((uint16_t)(timerNow() - last) >= interval) {
            last += interval;
            angle += direction * STEP_DEG;

            if (angle == -90 || angle == 90)
                direction = -direction;

            servoWrite(&servo, angle);
        }
    }
}
