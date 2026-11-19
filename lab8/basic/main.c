#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"

#define BUTTON_PIN PIN_RB0
#define SERVO_PIN  PIN_RC2

void main(void) {
    Button button;
    Servo servo;

    halBegin();
    buttonBegin(&button, BUTTON_PIN, false, 20);
    servoAttach(&servo, SERVO_PIN, -90, 90, 500, 2400);

	int16_t angle = -90;
	servoWrite(&servo, angle);
    while (1) {
        if (buttonReleased(&button)) {
			angle += 45;
			if (angle > 90) angle = -90;
            servoWrite(&servo, angle);
		}
    }
}
