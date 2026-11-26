#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"
#include <string.h>

#define TICK_MS 10U

static const Pin leds[] = {PIN_RA1, PIN_RA2, PIN_RA3, PIN_RA4};

enum Status {
	UP_RUNNING,
	UP_IDLE,
	DOWN_RUNNING,
	DOWN_IDLE
};

static uint8_t status = UP_RUNNING;
static uint8_t speed = 1000U / TICK_MS;
static uint8_t remaining;
static char input[100];
static uint8_t input_idx;
static volatile uint8_t cnt;

void __interrupt(high_priority) isr(void) {
	bool tick = timerInterruptFired(TIMER2);
	bool restart = false;
	uint8_t byte;
	if (uartRead(&byte)) {
		uartWrite(byte);
		if (byte == '\n' || byte == '\r') {
			if (input_idx && input_idx < sizeof input) {
				input[input_idx] = '\0';
				if (!strcmp(input, "START")) {
					if (status <= UP_IDLE) status = UP_RUNNING;
					else status = DOWN_RUNNING;
					restart = true;
				} else if (!strcmp(input, "STOP")) {
					if (status <= UP_IDLE) status = UP_IDLE;
					else status = DOWN_IDLE;
				} else if (!strcmp(input, "UP")) {
					if (status & 1U) status = UP_IDLE;
					else status = UP_RUNNING;
				} else if (!strcmp(input, "DOWN")) {
					if (status & 1U) status = DOWN_IDLE;
					else status = DOWN_RUNNING;
				} else if (input_idx == 9U && !strncmp(input, "SPEED ", 6U) && input[7] == '.') {
					if (input[6] == '1' && input[8] == '0') {
						speed = 1000U / TICK_MS;
					} else if (input[6] == '0' && input[8] >= '1' && input[8] <= '9') {
						speed = (uint8_t)((input[8] - '0') * (100U / TICK_MS));
					}
					restart = true;
				}
			}
			input_idx = 0;
		} else if (input_idx < sizeof(input) - 1U && byte != '\0') {
			input[input_idx++] = (char)byte;
		} else {
			input_idx = sizeof(input);
		}
	}

	if (restart) {
		// The first Timer2 tick may arrive less than TICK_MS after the command.
		remaining = speed + 1U;
	} else if (tick && !(status & 1U) && --remaining == 0U) {
		remaining = speed;
		if (status == UP_RUNNING) cnt = (cnt + 1U) % 16U;
		else cnt = (cnt + 15U) % 16U;
	}
}

void main(void) {
	uint8_t displayed = 0;
	halBegin();
	for (uint8_t i = 0; i < 4U; ++i) pinMode(leds[i], OUTPUT);
	digitalWriteBits(leds, 4, displayed);

	remaining = speed;
	if (!uartBegin(9600) || !timerAttachInterrupt(TIMER2, TICK_MS, PRIORITY_HIGH)) {
		for (;;) { NOP(); }
	}
	uartAttachInterrupt(PRIORITY_HIGH);

	while (1) {
		uint8_t current = cnt;
		if (current != displayed) {
			digitalWriteBits(leds, 4, current);
			displayed = current;
		}
	}
}
