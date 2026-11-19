#ifndef PIC18F4520_HAL_H
#define PIC18F4520_HAL_H

/*
 * PIC18F4520 HAL (XC8 / C99) -- single-header, no additional .c file.
 * Include this file in ONE application .c file per firmware project.
 *
 * #define HAL_FOSC_HZ 500000UL   // optional, before #include
 * #include "pic18f4520_hal.h"
 *
 * Configuration bits are intentionally not in this header.
 * Add pic18f4520_config.c to each firmware project exactly once.
 * Since this is a header-only HAL, its implementation is static inline.
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef HAL_FOSC_HZ
#define HAL_FOSC_HZ 500000UL
#endif

/* Internal oscillator choices supported by this HAL. */
#if   HAL_FOSC_HZ == 125000UL
#define HAL_IRCF 1U
#define HAL_T1_PS 0U
#define HAL_TICK_US 32UL
#elif HAL_FOSC_HZ == 250000UL
#define HAL_IRCF 2U
#define HAL_T1_PS 0U
#define HAL_TICK_US 16UL
#elif HAL_FOSC_HZ == 500000UL
#define HAL_IRCF 3U
#define HAL_T1_PS 0U
#define HAL_TICK_US 8UL
#elif HAL_FOSC_HZ == 1000000UL
#define HAL_IRCF 4U
#define HAL_T1_PS 0U
#define HAL_TICK_US 4UL
#elif HAL_FOSC_HZ == 2000000UL
#define HAL_IRCF 5U
#define HAL_T1_PS 1U
#define HAL_TICK_US 4UL
#elif HAL_FOSC_HZ == 4000000UL
#define HAL_IRCF 6U
#define HAL_T1_PS 2U
#define HAL_TICK_US 4UL
#elif HAL_FOSC_HZ == 8000000UL
#define HAL_IRCF 7U
#define HAL_T1_PS 3U
#define HAL_TICK_US 4UL
#else
#error "Unsupported HAL_FOSC_HZ (use 125k/250k/500k/1M/2M/4M/8M)."
#endif

/* Compile-time check for applications that require 50-Hz servo PWM.
 * The 8-bit Timer2 uses a maximum 1:16 prescaler. This macro can be
 * used in a #if directive in a servo application; 1 MHz is unsupported.
 * General-purpose PWM still checks runtime frequency in pwmBegin().
 */
#define HAL_SERVO_50HZ_SUPPORTED \
    (((HAL_FOSC_HZ + 1600UL) / 3200UL) >= 1UL && \
     ((HAL_FOSC_HZ + 1600UL) / 3200UL) <= 256UL)

/* Pin names are MCU port names, NOT physical DIP package pin numbers. */
typedef enum {
    PIN_RA0, PIN_RA1, PIN_RA2, PIN_RA3, PIN_RA4, PIN_RA5, PIN_RA6, PIN_RA7,
    PIN_RB0, PIN_RB1, PIN_RB2, PIN_RB3, PIN_RB4, PIN_RB5, PIN_RB6, PIN_RB7,
    PIN_RC0, PIN_RC1, PIN_RC2, PIN_RC3, PIN_RC4, PIN_RC5, PIN_RC6, PIN_RC7,
    PIN_RD0, PIN_RD1, PIN_RD2, PIN_RD3, PIN_RD4, PIN_RD5, PIN_RD6, PIN_RD7,
    PIN_RE0, PIN_RE1, PIN_RE2,
    PIN_NONE = 255
} Pin;

typedef enum { INPUT, OUTPUT } PinMode;
typedef enum { LOW = 0, HIGH = 1 } Level;

#define HAL_SET_BIT(reg, mask, enabled) \
    do { if (enabled) (reg) |= (mask); else (reg) &= (uint8_t)~(mask); } while (0)

static inline bool halValidPin(Pin pin) { return (unsigned)pin <= (unsigned)PIN_RE2; }
static inline uint8_t halPinMask(Pin pin) { return (uint8_t)(1U << ((unsigned)pin & 7U)); }

static inline void halBegin(void) {
    OSCCONbits.IRCF = HAL_IRCF;
    OSCCONbits.SCS = 2U;       /* Run from internal oscillator. */
    ADCON1 = 0x0FU;           /* Default: digital pins. */
    CMCON = 0x07U;            /* Comparators off. */
    INTCON2bits.RBPU = 1U;    /* Disable PORTB weak pull-ups. */
    CCP1CON = CCP2CON = T2CON = 0U; /* Reset PWM modules. */

    T1CON = (uint8_t)(0x80U | (HAL_T1_PS << 4)); /* RD16; Fosc/4. */
    TMR1H = 0U;
    TMR1L = 0U; /* RD16: write high first, then low. */
    T1CONbits.TMR1ON = 1U;
}

/* --- GPIO ---------------------------------------------------------- */
static inline bool pinMode(Pin pin, PinMode mode) {
    if (!halValidPin(pin) || (pin == PIN_RA5 && mode == OUTPUT)) return false;
    uint8_t mask = halPinMask(pin);
    bool in = mode == INPUT;
    switch ((unsigned)pin >> 3) {
        case 0: HAL_SET_BIT(TRISA, mask, in); break;
        case 1: HAL_SET_BIT(TRISB, mask, in); break;
        case 2: HAL_SET_BIT(TRISC, mask, in); break;
        case 3: HAL_SET_BIT(TRISD, mask, in); break;
        case 4: HAL_SET_BIT(TRISE, mask, in); break;
        default: return false;
    }
    return true;
}

static inline bool digitalWrite(Pin pin, Level value) {
    if (!halValidPin(pin)) return false;
    uint8_t mask = halPinMask(pin);
    bool on = value == HIGH;
    switch ((unsigned)pin >> 3) {
        case 0: HAL_SET_BIT(LATA, mask, on); break;
        case 1: HAL_SET_BIT(LATB, mask, on); break;
        case 2: HAL_SET_BIT(LATC, mask, on); break;
        case 3: HAL_SET_BIT(LATD, mask, on); break;
        case 4: HAL_SET_BIT(LATE, mask, on); break;
        default: return false;
    }
    return true;
}

static inline bool digitalRead(Pin pin) {
    if (!halValidPin(pin)) return false;
    uint8_t mask = halPinMask(pin);
    switch ((unsigned)pin >> 3) {
        case 0: return (PORTA & mask) != 0U;
        case 1: return (PORTB & mask) != 0U;
        case 2: return (PORTC & mask) != 0U;
        case 3: return (PORTD & mask) != 0U;
        case 4: return (PORTE & mask) != 0U;
        default: return false;
    }
}

static inline bool digitalToggle(Pin pin) {
    if (!halValidPin(pin)) return false;
    uint8_t mask = halPinMask(pin);
    switch ((unsigned)pin >> 3) {
        case 0: LATA ^= mask; break;
        case 1: LATB ^= mask; break;
        case 2: LATC ^= mask; break;
        case 3: LATD ^= mask; break;
        case 4: LATE ^= mask; break;
        default: return false;
    }
    return true;
}

/* Least-significant bit goes to pins[0], useful for Lab 9 LEDs. */
static inline void digitalWriteBits(const Pin *pins, uint8_t count, uint16_t bits) {
    for (uint8_t i = 0; i < count && i < 16U; ++i)
        digitalWrite(pins[i], (bits & ((uint16_t)1U << i)) ? HIGH : LOW);
}

static inline void portbPullups(bool enabled) { INTCON2bits.RBPU = !enabled; }

/* --- Timer1: polling scheduler, no interrupts ---------------------- */
static inline uint16_t timerNow(void) {
    /* RD16 mode: reading TMR1L latches TMR1H. */
    uint8_t lo = TMR1L;
    return (uint16_t)(((uint16_t)TMR1H << 8) | lo);
}

static inline uint16_t timerTicks(uint16_t ms, uint16_t steps) {
    if (!steps) return 0U;
    uint32_t divisor = (uint32_t)steps * HAL_TICK_US;
    uint32_t ticks = ((uint32_t)ms * 1000UL + divisor / 2UL) / divisor;
    return ticks > 65535UL ? 0U : (uint16_t)ticks;
}

/* Scheduler intervals must be shorter than one 16-bit Timer1 wrap. */
static inline void delayMs(uint16_t ms) {
    while (ms) {
        uint16_t chunk = ms > 100U ? 100U : ms;
        uint16_t last = timerNow();
        uint16_t period = timerTicks(chunk, 1U);
        while ((uint16_t)(timerNow() - last) < period) { }
        ms -= chunk;
    }
}

/* --- Button: one shared debounced state machine -------------------- */
/* Edges are latched independently, then consumed by Pressed/Released.  */
typedef struct {
    Pin pin;
    bool active_high;
    bool raw_down, stable_down;
    bool pending_press, pending_release;
    uint16_t changed_at, debounce_ticks;
} Button;

static inline void buttonBegin(Button *button, Pin pin,
                               bool active_high, uint16_t debounce_ms) {
    if (!button) return;
    pinMode(pin, INPUT);
    button->pin = pin;
    button->active_high = active_high;
    button->raw_down = (digitalRead(pin) == active_high);
    button->stable_down = button->raw_down;
    button->pending_press = false;
    button->pending_release = false;
    button->changed_at = timerNow();
    button->debounce_ticks = timerTicks(debounce_ms, 1U);
}

/* Call once per loop (or use any of the three button queries below). */
static inline void buttonUpdate(Button *button) {
    if (!button) return;
    bool raw = (digitalRead(button->pin) == button->active_high);
    uint16_t now = timerNow();
    if (raw != button->raw_down) {
        button->raw_down = raw;
        button->changed_at = now;
    }
    if (raw != button->stable_down &&
        (uint16_t)(now - button->changed_at) >= button->debounce_ticks) {
        button->stable_down = raw;
        if (raw) button->pending_press = true;
        else     button->pending_release = true;
    }
}

/* Each edge fires once, even if another query was called first. */
static inline bool buttonPressed(Button *button) {
    if (!button) return false;
    buttonUpdate(button);
    bool result = button->pending_press;
    button->pending_press = false;
    return result;
}

static inline bool buttonReleased(Button *button) {
    if (!button) return false;
    buttonUpdate(button);
    bool result = button->pending_release;
    button->pending_release = false;
    return result;
}

/* Level: true for as long as the debounced button is pressed. */
static inline bool buttonHeld(Button *button) {
    if (!button) return false;
    buttonUpdate(button);
    return button->stable_down;
}

/* --- CCP hardware PWM: RC2 (CCP1), RC1 (CCP2 with CCP2MX=PORTC) --- */
static inline bool halPwmPin(Pin pin) { return pin == PIN_RC2 || pin == PIN_RC1; }
static inline bool halPwmActive(Pin pin) {
    if (pin == PIN_RC2) return (CCP1CON & 0x0FU) == 0x0CU;
    if (pin == PIN_RC1) return (CCP2CON & 0x0FU) == 0x0CU;
    return false;
}

static inline uint16_t halPwmMaxCount(void) {
    return (uint16_t)(4U * ((uint16_t)PR2 + 1U));
}

static inline uint8_t halPwmPrescale(void) {
    uint8_t ps = T2CONbits.T2CKPS;
    return ps == 0U ? 1U : (ps == 1U ? 4U : 16U);
}

static inline bool pwmBegin(Pin pin, uint16_t frequency_hz) {
    if (!halPwmPin(pin) || frequency_hz == 0U) return false;
    const uint8_t scales[3] = {1U, 4U, 16U};
    uint16_t counts = 0U;
    uint8_t choice = 0U;
    for (uint8_t i = 0U; i < 3U; ++i) {
        uint32_t denom = 4UL * scales[i] * frequency_hz;
        uint32_t candidate = (HAL_FOSC_HZ + denom / 2UL) / denom;
        if (candidate >= 1U && candidate <= 256U) {
            counts = (uint16_t)candidate;
            choice = i;
            break;
        }
    }
    if (!counts) return false;    /* This clock cannot produce this PWM rate. */

    /* CCP1 and CCP2 share Timer2. Do not silently change another PWM. */
    bool in_use = halPwmActive(PIN_RC2) || halPwmActive(PIN_RC1);
    if (in_use && ((uint16_t)PR2 + 1U != counts ||
                   T2CONbits.T2CKPS != choice)) return false;

    if (!in_use) {
        T2CON = 0U;
        TMR2 = 0U;
        PR2 = (uint8_t)(counts - 1U);
        T2CONbits.T2CKPS = choice;
    }
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    if (pin == PIN_RC2) { CCPR1L = 0U; CCP1CON = 0x0CU; }
    else                { CCPR2L = 0U; CCP2CON = 0x0CU; }
    T2CONbits.TMR2ON = 1U;
    return true;
}

static inline bool halPwmWriteCount(Pin pin, uint16_t count) {
    if (!halPwmActive(pin)) return false;
    uint16_t max = halPwmMaxCount();
    if (count > max) count = max;
    if (count > 1023U) count = 1023U; /* PWM has a 10-bit duty register. */
    uint8_t low = (uint8_t)((count & 3U) << 4);
    if (pin == PIN_RC2) {
        CCPR1L = (uint8_t)(count >> 2);
        CCP1CON = (uint8_t)((CCP1CON & 0xCFU) | low);
    } else {
        CCPR2L = (uint8_t)(count >> 2);
        CCP2CON = (uint8_t)((CCP2CON & 0xCFU) | low);
    }
    return true;
}

static inline bool pwmWriteMicros(Pin pin, uint16_t high_us) {
    if (!halPwmActive(pin)) return false;
    /* HAL_FOSC_HZ values are integral multiples of 1000 Hz. */
    uint32_t numerator = (uint32_t)high_us * (HAL_FOSC_HZ / 1000UL);
    uint32_t denominator = (uint32_t)halPwmPrescale() * 1000UL;
    uint32_t count = (numerator + denominator / 2UL) / denominator;
    return halPwmWriteCount(pin, (uint16_t)(count > 1023UL ? 1023UL : count));
}

static inline bool pwmWriteDuty(Pin pin, uint16_t duty_0_to_1023) {
    if (duty_0_to_1023 > 1023U || !halPwmActive(pin)) return false;
    uint16_t max = halPwmMaxCount();
    uint16_t count = (uint16_t)(((uint32_t)duty_0_to_1023 * max + 511UL) / 1023UL);
    return halPwmWriteCount(pin, count);
}

/* --- Servo: configurable angle range + pulse-width calibration ---- */
typedef struct {
    Pin pin;
    int16_t min_angle, max_angle, angle;
    uint16_t min_us, max_us;
} Servo;

static inline bool servoWrite(Servo *servo, int16_t angle) {
    if (!servo || servo->max_angle <= servo->min_angle) return false;
    if (angle < servo->min_angle) angle = servo->min_angle;
    if (angle > servo->max_angle) angle = servo->max_angle;
    uint32_t fraction = (uint32_t)(angle - servo->min_angle);
    uint32_t span = (uint32_t)(servo->max_angle - servo->min_angle);
    uint16_t high_us = (uint16_t)(servo->min_us +
        (fraction * (servo->max_us - servo->min_us) + span / 2UL) / span);
    if (!pwmWriteMicros(servo->pin, high_us)) return false;
    servo->angle = angle;
    return true;
}

static inline bool servoAttach(Servo *servo, Pin pin,
                               int16_t min_angle, int16_t max_angle,
                               uint16_t min_us, uint16_t max_us) {
    if (!servo || max_angle <= min_angle || max_us <= min_us) return false;
    if (!pwmBegin(pin, 50U)) return false;
    servo->pin = pin;
    servo->min_angle = min_angle;
    servo->max_angle = max_angle;
    servo->min_us = min_us;
    servo->max_us = max_us;
    return servoWrite(servo, min_angle);
}

/* --- ADC: analog channels are configured in groups by ADCON1 ----- */
static inline int8_t analogChannel(Pin pin) {
    switch (pin) {
        case PIN_RA0: return 0;  case PIN_RA1: return 1;
        case PIN_RA2: return 2;  case PIN_RA3: return 3;
        case PIN_RA5: return 4;  case PIN_RE0: return 5;
        case PIN_RE1: return 6;  case PIN_RE2: return 7;
        case PIN_RB2: return 8;  case PIN_RB3: return 9;
        case PIN_RB1: return 10; case PIN_RB4: return 11;
        case PIN_RB0: return 12;
        default: return -1;
    }
}

/* Configures AN0 through selected channel analog, rest digital.
 * If highest_pin=RB0/AN12, RB0 can no longer act as a digital button. */
static inline bool analogBegin(Pin highest_pin) {
    int8_t channel = analogChannel(highest_pin);
    if (channel < 0) return false;
    ADCON1 = (uint8_t)(channel == 12 ? 0 : 14 - channel);
    ADCON2 = 0xBFU;  /* ADFM=1, ACQT=20 TAD, ADCS=FRC. */
    ADCON0 = 0x01U;  /* ADC on, start with AN0 selected. */
    return true;
}

static inline bool analogRead(Pin pin, uint16_t *result) {
    int8_t channel = analogChannel(pin);
    if (!result || channel < 0 || (ADCON1 & 0x0FU) == 0x0FU) return false;
    if ((int)channel > 14 - (int)(ADCON1 & 0x0FU)) return false;
    pinMode(pin, INPUT);
    ADCON0 = (uint8_t)(((uint8_t)channel << 2) | 0x01U);
    ADCON0 |= 0x02U;        /* GO/DONE = 1. */
    while (ADCON0 & 0x02U) { }
    *result = (uint16_t)(((uint16_t)ADRESH << 8) | ADRESL);
    return true;
}

/* Maps 0..1023 to 0..(sections-1), including the 1023 endpoint. */
static inline uint8_t analogBucket(uint16_t raw, uint8_t sections) {
    if (!sections) return 0U;
    if (raw > 1023U) raw = 1023U;
    return (uint8_t)(((uint32_t)raw * sections) / 1024UL);
}

static inline uint16_t analogMillivolts(uint16_t raw, uint16_t vref_mv) {
    if (raw > 1023U) raw = 1023U;
    return (uint16_t)(((uint32_t)raw * vref_mv + 511UL) / 1023UL);
}

#endif /* PIC18F4520_HAL_H */
