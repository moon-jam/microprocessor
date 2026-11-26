#include "../../lib/pic18f4520_config.h"
#include "../../lib/pic18f4520_hal.h"
#include <string.h>
#include <stdio.h>

static const Pin leds[] = {PIN_RA1, PIN_RA2, PIN_RA3, PIN_RA4};

int32_t operand[4];
char operator[3];
int idx = 0;
int n = 0; // number of operator
char input[100];
char finish = 0;

int32_t cal() {
	// a [+-*] b [+-*] c [+-*] d
	for(int i = 0; i < n; ++i) {
		int j = i;
		while(j < n && operator[j] == '*') operand[i] *= operand[++j];
		i = j;
	}
	int32_t res = operand[0];
	for(int i = 0; i < n; ++i) {
		if(operator[i] == '+') res += operand[i+1];
		if(operator[i] == '-') res -= operand[i+1];
	}
	return res;
}

void reset() {
    printf("\r\033[2K");
    idx = 0;
	n = 0;
	finish = 0;
	memset(input, 0, sizeof(input));
	memset(operand, 0, sizeof(operand));
	memset(operator, 0, sizeof(operator));
    digitalWriteBits(leds, 4, 0);
}

void main() {
	halBegin();
	for (int i = 0 ; i < 4; ++i) pinMode(leds[i], OUTPUT);
	uartBegin(9600);
	reset();

	while (1) {
		if (uartRead(input+idx)) {
			if (input[idx] >= '0' && input[idx] <= '9') {
				uartWrite(input[idx]);
				operand[n] = input[idx]-'0';
				idx++;
				digitalWriteBits(leds, 4, (uint16_t)operand[n]);
			} else reset();
			while(!input[idx]) uartRead(input+idx);
			if (input[idx] >= '0' && input[idx] <= '9') {
				uartWrite(input[idx]);
				operand[n] = input[idx]-'0' + operand[n]*10;
				digitalWriteBits(leds, 4, (uint16_t)operand[n]);
				idx++;
				while(!input[idx]) uartRead(input+idx);
			}
			if (input[idx] == '+' || input[idx] == '-' || input[idx] == '*') {
				if(n == 3) { reset(); continue; }
				uartWrite(input[idx]);
				operator[n++] = input[idx];
				idx++;
				digitalWriteBits(leds, 4, 0);
			} else if (input[idx] == '\r' || input[idx] == '\n') {
				finish = 1;
				int32_t res = cal();
				digitalWriteBits(leds, 4, (uint16_t)res);
				printf("=%ld\r\n", res);
				while(uartRead(input+idx));
				idx++;
			} 
			else reset();
		}
		if (finish) {
			while(input[idx] != '\r' && input[idx] != '\n') {
				uartRead(input+idx);
			}
			reset();
		}
	}
}
