#include <xc.h>
#include <stdint.h>
#define _XTAL_FREQ 1000000
#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF

char state = 0;
char stage = 0;

void update_led(void) {
    switch (state) {
        case 0:
            LATA = 0;
            break;

        case 1:
            // RA0 -> RA1 -> RA2, every 500 ms
			stage %= 3;
			if (stage == 0) LATA = 1;
			if (stage == 1) LATA = 2;
			if (stage == 2) LATA = 4;
            break;

        case 2:
            // RA0 1s -> RA1 1s -> RA2 flashing 2.5s
			stage %= 9;
			if (stage <= 1) LATA = 1;
			else if (stage <= 3) LATA = 3;
			else if (stage%2==0) LATA = 4;
			else LATA = 0;
            break;
    }
}

void init(void) {
    ADCON1 = 0x0F;

    TRISAbits.TRISA0 = 0;
    TRISAbits.TRISA1 = 0;
    TRISAbits.TRISA2 = 0;

    TRISBbits.TRISB0 = 1;

    LATA = 0;
}

int main() {
	init();
	unsigned int clk = 0;
	unsigned int last_press = 0;
	unsigned int tick = 0;
	while (1) {
		if (PORTBbits.RB0 == 0 && clk - last_press >= 5) {
			state = (1+state)%3;
			stage = 0;
			tick = 0;
			update_led();
		}
		if (PORTBbits.RB0 == 0) last_press = clk;
		if (++tick >= 50) {
			tick = 0;
			stage++;
			update_led();
		}
		clk++;
		__delay_ms(10);
	}
	return 0;
}