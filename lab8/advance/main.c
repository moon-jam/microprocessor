#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"

#define BUTTON_PIN  PIN_RB0
#define SERVO_PIN   PIN_RC2
#define STEP_TICKS  (5000000UL / (360UL * HAL_TICK_US)) // 5,000,000 us / 360 ticks

void main(void) {
    Button button;
    Servo servo;
    bool running = false;
    int16_t angle = -90;
    int8_t direction = 1;
    uint16_t last;

    halBegin();
    buttonBegin(&button, BUTTON_PIN, false, 20); // Active Low
    servoAttach(&servo, SERVO_PIN, -90, 90, 500, 2400);

    last = timerNow();

	servoWrite(&servo, angle);
    while (1) {
        if (buttonReleased(&button)) {
            running = !running;
            last = timerNow();
        }

        if (running && (uint16_t)(timerNow() - last) >= STEP_TICKS) {
            last += STEP_TICKS;
            angle += direction;

            if (angle == -90 || angle == 90)
                direction = -direction;

            servoWrite(&servo, angle);
        }
    }
}