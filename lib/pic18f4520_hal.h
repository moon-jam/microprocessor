#ifndef PIC18F4520_HAL_H
#define PIC18F4520_HAL_H

/**
 * @file pic18f4520_hal.h
 * @brief Single-header hardware abstraction layer for the PIC18F4520 on XC8.
 *
 * Include this header in one C source file per firmware project. It defines
 * putch() and getch() with external linkage for XC8 standard output and input.
 * Include pic18f4520_config.h once to supply the device configuration bits.
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef HAL_FOSC_HZ
/**
 * Internal oscillator frequency in Hz. Override before including this header.
 * Supported values are 125000, 250000, 500000, 1000000, 2000000, 4000000,
 * and 8000000.
 */
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

/**
 * @brief Nonzero if Timer2 can generate 50 Hz pulse-width modulation (PWM).
 * @note This compile-time check does not account for Timer2 already being in use.
 */
#define HAL_SERVO_50HZ_SUPPORTED \
    (((HAL_FOSC_HZ + 1600UL) / 3200UL) >= 1UL && \
     ((HAL_FOSC_HZ + 1600UL) / 3200UL) <= 256UL)

/** Microcontroller port and bit identifiers, independent of package pin numbering. */
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

/**
 * @brief Initialize the HAL and start Timer1 as its time base.
 *
 * Selects the configured internal oscillator, sets analog-capable pins to
 * digital mode, disables PORTB weak pull-ups, resets PWM, and enables interrupt
 * priority mode. Call once at startup before configuring any HAL peripheral.
 */
static inline void halBegin(void) {
    OSCCONbits.IRCF = HAL_IRCF;
    OSCCONbits.SCS = 2U;       /* Run from internal oscillator. */
    ADCON1 = 0x0FU;           /* Default: digital pins. */
    CMCON = 0x07U;            /* Comparators off. */
    INTCON2bits.RBPU = 1U;    /* Disable PORTB weak pull-ups. */
    RCONbits.IPEN = 1U;
    CCP1CON = CCP2CON = T2CON = 0U; /* Reset PWM modules. */

    T1CON = (uint8_t)(0x80U | (HAL_T1_PS << 4)); /* RD16; Fosc/4. */
    TMR1H = 0U;
    TMR1L = 0U; /* RD16: write high first, then low. */
    T1CONbits.TMR1ON = 1U;
}

/* --- GPIO ---------------------------------------------------------- */

/**
 * @brief Set the direction of a digital pin.
 * @param mode `INPUT` or `OUTPUT`.
 * @return False for an invalid pin or when requesting output on `PIN_RA5`.
 */
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

/**
 * @brief Set the pin's output latch to `HIGH` or `LOW`.
 * @pre Configure the pin as an output for the latch to drive the pin.
 * @return False if the pin identifier is invalid.
 */
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

/**
 * @brief Read the electrical level of a digital pin.
 * @return True for high. Low and invalid pin identifiers both return false.
 */
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

/**
 * @brief Invert the pin's output latch.
 * @pre Configure the pin as an output for the latch to drive the pin.
 * @return False if the pin identifier is invalid.
 */
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

/**
 * @brief Write up to 16 output pins from the corresponding bits of a value.
 * @param pins Array of configured output pins. Element 0 receives the least significant bit.
 * @param count Number of array elements to write, capped at 16.
 */
static inline void digitalWriteBits(const Pin *pins, uint8_t count, uint16_t bits) {
    for (uint8_t i = 0; i < count && i < 16U; ++i)
        digitalWrite(pins[i], (bits & ((uint16_t)1U << i)) ? HIGH : LOW);
}

/** Enable or disable internal weak pull-ups on PORTB inputs. */
static inline void portbPullups(bool enabled) { INTCON2bits.RBPU = !enabled; }

/* --- Timer1: polling scheduler ------------------------------------- */

/**
 * @brief Read the free-running 16-bit Timer1 count configured by halBegin().
 *
 * Use unsigned 16-bit subtraction to measure elapsed counts within one wrap.
 * Keep Timer1 accesses in one execution context because its high-byte latch
 * is shared. Keep Timer1 free-running when using it as the HAL time base.
 */
static inline uint16_t timerNow(void) {
    /* RD16 mode: reading TMR1L latches TMR1H. */
    uint8_t lo = TMR1L;
    return (uint16_t)(((uint16_t)TMR1H << 8) | lo);
}

/**
 * @brief Convert one step of a millisecond interval into Timer1 counts.
 * @param ms Total interval in milliseconds.
 * @param steps Number of equal steps in the interval. Pass 1 for the whole interval.
 * @return Counts per step rounded to the nearest count. Returns 0 when
 *         `steps` is 0 or the result exceeds 65535 counts. Short intervals
 *         can also round to 0.
 */
static inline uint16_t timerTicks(uint16_t ms, uint16_t steps) {
    if (!steps) return 0U;
    uint32_t divisor = (uint32_t)steps * HAL_TICK_US;
    uint32_t ticks = ((uint32_t)ms * 1000UL + divisor / 2UL) / divisor;
    return ticks > 65535UL ? 0U : (uint16_t)ticks;
}

/**
 * @brief Wait for `ms` milliseconds using the free-running Timer1 time base.
 * @note This call blocks the current execution context. Use it outside an interrupt handler.
 */
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

/** Debouncer with one pending event per edge. Repeated unconsumed edges coalesce. */
typedef struct {
    Pin pin;
    bool active_high;
    bool raw_down, stable_down;
    bool pending_press, pending_release;
    uint16_t changed_at, debounce_ticks;
} Button;

/**
 * @brief Configure a button input and record its initial level.
 * @param[out] button Button state to initialize.
 * @param pin Digital input connected to the button.
 * @param active_high True if pressing the button makes the input high.
 * @param debounce_ms Time the input must remain stable, in milliseconds.
 *        Use an interval representable by timerTicks() with one step.
 * @pre Keep Timer1 configured as the HAL time base and bias the idle input level.
 */
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

/** Refresh a button's latched events. Each button query also refreshes the state. */
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

/** Poll an initialized button and return true if a debounced press event was consumed. */
static inline bool buttonPressed(Button *button) {
    if (!button) return false;
    buttonUpdate(button);
    bool result = button->pending_press;
    button->pending_press = false;
    return result;
}

/** Poll an initialized button and return true if a debounced release event was consumed. */
static inline bool buttonReleased(Button *button) {
    if (!button) return false;
    buttonUpdate(button);
    bool result = button->pending_release;
    button->pending_release = false;
    return result;
}

/** Return true while the debounced button is held. Edge events remain pending. */
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

/**
 * @brief Start pulse-width modulation with a duty cycle of zero.
 * @param pin `PIN_RC2` for CCP1 or `PIN_RC1` for CCP2 with `CCP2MX = PORTC`.
 * @param frequency_hz Requested rate in Hz, rounded to a period Timer2 can generate.
 * @return True on success. Returns false for an unsupported pin or frequency,
 *         an enabled Timer2 interrupt, or a period mismatch with an active channel.
 */
static inline bool pwmBegin(Pin pin, uint16_t frequency_hz) {
    if (!halPwmPin(pin) || frequency_hz == 0U || PIE1bits.TMR2IE) return false;
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

/**
 * @brief Set how long an active PWM output stays high in each period.
 * @param high_us High time in microseconds, clamped to the supported range.
 * @return False if the pin has no active PWM output.
 */
static inline bool pwmWriteMicros(Pin pin, uint16_t high_us) {
    if (!halPwmActive(pin)) return false;
    /* HAL_FOSC_HZ values are integral multiples of 1000 Hz. */
    uint32_t numerator = (uint32_t)high_us * (HAL_FOSC_HZ / 1000UL);
    uint32_t denominator = (uint32_t)halPwmPrescale() * 1000UL;
    uint32_t count = (numerator + denominator / 2UL) / denominator;
    return halPwmWriteCount(pin, (uint16_t)(count > 1023UL ? 1023UL : count));
}

/**
 * @brief Set the duty cycle of an active PWM output on a 0 to 1023 scale.
 * @return False if the pin has no active PWM output or the input exceeds 1023.
 */
static inline bool pwmWriteDuty(Pin pin, uint16_t duty_0_to_1023) {
    if (duty_0_to_1023 > 1023U || !halPwmActive(pin)) return false;
    uint16_t max = halPwmMaxCount();
    uint16_t count = (uint16_t)(((uint32_t)duty_0_to_1023 * max + 511UL) / 1023UL);
    return halPwmWriteCount(pin, count);
}

/* --- Servo: configurable angle range + pulse-width calibration ---- */

/** Servo calibration and last commanded angle in degrees. */
typedef struct {
    Pin pin;
    int16_t min_angle, max_angle, angle;
    uint16_t min_us, max_us;
} Servo;

/**
 * @brief Set an attached servo's target angle in degrees.
 * @param angle Target angle, clamped to the calibrated range.
 * @return True if the PWM output accepted the command.
 */
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

/**
 * @brief Start a 50 Hz servo output and command the minimum angle.
 * @param[out] servo Servo state to initialize.
 * @param pin PWM output pin supported by pwmBegin().
 * @param min_angle Lower angle endpoint in degrees.
 * @param max_angle Upper angle endpoint in degrees, greater than `min_angle`.
 * @param min_us Calibrated pulse width in microseconds at `min_angle`.
 * @param max_us Calibrated pulse width in microseconds at `max_angle`, greater than `min_us`.
 * @return False if calibration arguments are invalid or PWM is unavailable.
 */
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

/** Return the analog channel number for a pin, or -1 if the pin has no channel. */
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

/**
 * @brief Enable analog inputs from AN0 through the channel of `highest_pin`.
 *
 * Other analog-capable pins become digital. The voltage references are VSS
 * and VDD. Check the entire enabled group before using any of its pins as digital inputs.
 * @return False if `highest_pin` has no analog channel.
 */
static inline bool analogBegin(Pin highest_pin) {
    int8_t channel = analogChannel(highest_pin);
    if (channel < 0) return false;
    ADCON1 = (uint8_t)(channel == 12 ? 0 : 14 - channel);
    ADCON2 = 0xBFU;  /* ADFM=1, ACQT=20 TAD, ADCS=FRC. */
    ADCON0 = 0x01U;  /* ADC on, start with AN0 selected. */
    return true;
}

/**
 * @brief Perform a blocking 10-bit conversion on an enabled analog input.
 * @param[out] result Receives the reading from 0 to 1023 on success.
 * @return False if the pin has no enabled analog channel or `result` is null.
 */
static inline bool analogRead(Pin pin, uint16_t *result) {
    int8_t channel = analogChannel(pin);
    if (!result || channel < 0 || (ADCON1 & 0x0FU) == 0x0FU) return false;
    if ((int)channel > 14 - (int)(ADCON1 & 0x0FU)) return false;
    pinMode(pin, INPUT);
    ADCON0 = (uint8_t)(((unsigned int)channel << 2) | 0x01U);
    ADCON0 |= 0x02U;        /* GO/DONE = 1. */
    while (ADCON0 & 0x02U) { }
    *result = (uint16_t)(((uint16_t)ADRESH << 8) | ADRESL);
    return true;
}

/**
 * @brief Convert a reading to millivolts, rounding to the nearest millivolt.
 * @param raw ADC reading. Values above 1023 are clamped to 1023.
 * @param vref_mv Full-scale reference voltage in millivolts.
 */
static inline uint16_t analogMillivolts(uint16_t raw, uint16_t vref_mv) {
    if (raw > 1023U) raw = 1023U;
    return (uint16_t)(((uint32_t)raw * vref_mv + 511UL) / 1023UL);
}

/**
 * @brief Configure the universal asynchronous receiver/transmitter (UART).
 *
 * RC6 transmits and RC7 receives. The frame has eight data bits, no parity,
 * and one stop bit.
 * @param baud Requested baud rate within the hardware range.
 * @return True if the baud-rate generator can provide the requested rate
 *         with no more than 3% nominal error.
 */
static inline bool uartBegin(uint32_t baud) {
    if (!baud) return false;
    uint32_t divisor = (HAL_FOSC_HZ + 2UL * baud) / (4UL * baud);
    if (divisor == 0UL || divisor > 65536UL) return false;
    uint32_t actual = HAL_FOSC_HZ / (4UL * divisor);
    uint32_t error = actual > baud ? actual - baud : baud - actual;
    if (error * 100UL > baud * 3UL) return false;
    pinMode(PIN_RC6, INPUT);
    pinMode(PIN_RC7, INPUT);
    BAUDCONbits.BRG16 = 1U;
    SPBRGH = (uint8_t)((divisor - 1UL) >> 8);
    SPBRG = (uint8_t)(divisor - 1UL);
    TXSTA = 0U;
    TXSTAbits.BRGH = 1U;
    TXSTAbits.TXEN = 1U;
    RCSTA = 0U;
    RCSTAbits.CREN = 1U;
    RCSTAbits.SPEN = 1U;
    return true;
}

/**
 * @brief Wait for a transmit buffer slot and queue one byte.
 * @pre Initialize the peripheral with uartBegin().
 * @note The final bit may still be on the wire when the call returns.
 */
static inline void uartWrite(uint8_t byte) {
    while (!PIR1bits.TXIF) { }
    TXREG = byte;
}

/** Queue a null-terminated string with uartWrite(), without adding a newline. */
static inline void uartPrint(const char *text) {
    while (*text) uartWrite((uint8_t)*text++);
}

/** Send one character to the UART for the XC8 standard output stream. */
void putch(char c) { uartWrite((uint8_t)c); }

/**
 * @brief Read one received byte if available, without waiting.
 * @param[out] byte Receives the byte on success.
 * @return True if a byte was read. Returns false if none is ready or `byte` is null.
 * @note The receiver restarts after an overrun. Some incoming bytes may be lost.
 */
static inline bool uartRead(uint8_t *byte) {
    if (RCSTAbits.OERR) {
        RCSTAbits.CREN = 0U;
        RCSTAbits.CREN = 1U;
    }
    if (!byte || !PIR1bits.RCIF) return false;
    *byte = RCREG;
    return true;
}

/**
 * @brief Wait for one UART byte for the XC8 standard input stream.
 * @pre Initialize the receiver with uartBegin() and read it only from main code.
 */
int getch(void) {
    uint8_t byte;
    while (!uartRead(&byte)) { }
    return byte;
}

typedef enum { FALLING, RISING } Edge;
typedef enum { PRIORITY_HIGH, PRIORITY_LOW } Priority;
typedef enum { TIMER0, TIMER1, TIMER2, TIMER3 } Timer;

/**
 * @brief Enable global interrupts after halBegin() selects priority mode.
 * @param priority `PRIORITY_HIGH` enables high-priority delivery.
 *                 `PRIORITY_LOW` also enables low-priority delivery.
 * @note The attach helpers call this function after configuring their source.
 */
static inline void halEnableInterrupts(Priority priority) {
    INTCONbits.GIEH = 1U;
    if (priority == PRIORITY_LOW) INTCONbits.GIEL = 1U;
}

/**
 * @brief Configure an external interrupt, clear its flag, and enable it.
 * @param pin `PIN_RB0`, `PIN_RB1`, or `PIN_RB2`. RB0 is fixed at high priority.
 * @param edge `FALLING` for high-to-low or `RISING` for low-to-high.
 * @param priority Priority of the application's interrupt handler for this source.
 * @pre Define the handler before calling this function. It also enables global interrupts.
 * @return False for an unsupported pin or a low-priority request on RB0.
 */
static inline bool pinAttachInterrupt(Pin pin, Edge edge, Priority priority) {
    bool rising = edge == RISING;
    bool high = priority == PRIORITY_HIGH;
    if (pin == PIN_RB0 && !high) return false;
    if (pin != PIN_RB0 && pin != PIN_RB1 && pin != PIN_RB2) return false;
    pinMode(pin, INPUT);
    switch (pin) {
        case PIN_RB0:
            INTCON2bits.INTEDG0 = rising;
            INTCONbits.INT0IF = 0U;
            INTCONbits.INT0IE = 1U;
            break;
        case PIN_RB1:
            INTCON2bits.INTEDG1 = rising;
            INTCON3bits.INT1IP = high;
            INTCON3bits.INT1IF = 0U;
            INTCON3bits.INT1IE = 1U;
            break;
        case PIN_RB2:
            INTCON2bits.INTEDG2 = rising;
            INTCON3bits.INT2IP = high;
            INTCON3bits.INT2IF = 0U;
            INTCON3bits.INT2IE = 1U;
            break;
        default: break;
    }
    halEnableInterrupts(priority);
    return true;
}

/** Return true and clear the specified external interrupt flag if it is set. */
static inline bool pinInterruptFired(Pin pin) {
    switch (pin) {
        case PIN_RB0:
            if (!INTCONbits.INT0IF) return false;
            INTCONbits.INT0IF = 0U;
            return true;
        case PIN_RB1:
            if (!INTCON3bits.INT1IF) return false;
            INTCON3bits.INT1IF = 0U;
            return true;
        case PIN_RB2:
            if (!INTCON3bits.INT2IF) return false;
            INTCON3bits.INT2IF = 0U;
            return true;
        default: return false;
    }
}

/**
 * @brief Enable the UART receive interrupt at the requested priority.
 * @pre Configure the receiver with uartBegin() and define the corresponding interrupt handler.
 * @note This also enables global interrupts at the requested priority.
 * @see uartRead()
 */
static inline void uartAttachInterrupt(Priority priority) {
    IPR1bits.RCIP = priority == PRIORITY_HIGH;
    PIE1bits.RCIE = 1U;
    halEnableInterrupts(priority);
}

static uint16_t halTimer0Reload;
static uint16_t halTimer1Reload;
static uint16_t halTimer3Reload;

static inline bool halTimer2AttachInterrupt(uint32_t cycles, bool high) {
    if (cycles > 4UL * 256UL * 16UL * 16UL ||
        (CCP1CON & 0x0CU) == 0x0CU || (CCP2CON & 0x0CU) == 0x0CU) return false;

    uint32_t best_error = UINT32_MAX;
    uint8_t best_period = 0U, best_pre = 0U, best_post = 0U;
    for (uint8_t pre = 0U; pre < 3U && best_error; ++pre) {
        for (uint8_t post = 1U; post <= 16U && best_error; ++post) {
            uint16_t divider = (uint16_t)((4U << (2U * pre)) * post);
            uint32_t ticks = (cycles + divider / 2U) / divider;
            if (!ticks) ticks = 1UL;
            if (ticks > 256UL) ticks = 256UL;
            uint32_t actual = ticks * divider;
            uint32_t error = actual > cycles ? actual - cycles : cycles - actual;
            if (error >= best_error) continue;
            best_error = error;
            best_period = (uint8_t)(ticks - 1UL);
            best_pre = pre;
            best_post = (uint8_t)(post - 1U);
        }
    }
    if (best_error == UINT32_MAX) return false;

    PIE1bits.TMR2IE = 0U;
    T2CON = 0U;
    PR2 = best_period;
    TMR2 = 0U;
    T2CONbits.T2CKPS = best_pre;
    T2CONbits.T2OUTPS = best_post;
    IPR1bits.TMR2IP = high;
    PIR1bits.TMR2IF = 0U;
    PIE1bits.TMR2IE = 1U;
    T2CONbits.TMR2ON = 1U;
    return true;
}

/**
 * @brief Configure and start a periodic interrupt on the selected timer.
 * @param timer `TIMER0`, `TIMER1`, `TIMER2`, or `TIMER3`.
 * @param period_ms Nonzero requested period in milliseconds, rounded to hardware counts.
 * @param priority Priority of the application's interrupt handler for this source.
 * @pre Define the handler before calling this function. It also enables global interrupts.
 * @return True on success. Returns false for invalid arguments, a period
 *         beyond the selected timer's range, or PWM already using Timer2.
 *
 * Each timer is configured independently. Reattaching a timer replaces its settings.
 * @warning Using `TIMER1` here replaces the free-running time base for timerNow(),
 *          delayMs(), and Button. `TIMER2` is shared with PWM and servo outputs.
 * @see timerInterruptFired()
 */
static inline bool timerAttachInterrupt(Timer timer, uint16_t period_ms,
                                        Priority priority) {
    if ((unsigned)timer > TIMER3 || !period_ms ||
        (priority != PRIORITY_HIGH && priority != PRIORITY_LOW)) return false;
    uint32_t cycles = (uint32_t)period_ms * (HAL_FOSC_HZ / 1000UL);
    bool high = priority == PRIORITY_HIGH;
    if (timer == TIMER2) {
        if (!halTimer2AttachInterrupt(cycles, high)) return false;
        halEnableInterrupts(priority);
        return true;
    }

    uint8_t max_shift = timer == TIMER0 ? 8U : 3U;
    uint8_t shift = 0U;
    while (shift < max_shift && cycles > (262144UL << shift)) ++shift;
    if (cycles > (262144UL << shift)) return false;
    uint32_t divider = 4UL << shift;
    uint16_t reload = (uint16_t)(65536UL - (cycles + divider / 2UL) / divider);

    switch (timer) {
        case TIMER0:
            INTCONbits.TMR0IE = 0U;
            T0CON = 0U;
            if (shift) T0CONbits.T0PS = (uint8_t)(shift - 1U);
            else T0CONbits.PSA = 1U;
            halTimer0Reload = reload;
            TMR0H = (uint8_t)(reload >> 8);
            TMR0L = (uint8_t)reload;
            INTCON2bits.TMR0IP = high;
            INTCONbits.TMR0IF = 0U;
            INTCONbits.TMR0IE = 1U;
            T0CONbits.TMR0ON = 1U;
            break;
        case TIMER1:
            PIE1bits.TMR1IE = 0U;
            T1CONbits.TMR1ON = 0U;
            T1CONbits.TMR1CS = 0U;
            T1CONbits.RD16 = 1U;
            T1CONbits.T1CKPS = shift;
            halTimer1Reload = reload;
            TMR1H = (uint8_t)(reload >> 8);
            TMR1L = (uint8_t)reload;
            IPR1bits.TMR1IP = high;
            PIR1bits.TMR1IF = 0U;
            PIE1bits.TMR1IE = 1U;
            T1CONbits.TMR1ON = 1U;
            break;
        case TIMER3:
            PIE2bits.TMR3IE = 0U;
            T3CONbits.TMR3ON = 0U;
            T3CONbits.TMR3CS = 0U;
            T3CONbits.RD16 = 1U;
            T3CONbits.T3CKPS = shift;
            halTimer3Reload = reload;
            TMR3H = (uint8_t)(reload >> 8);
            TMR3L = (uint8_t)reload;
            IPR2bits.TMR3IP = high;
            PIR2bits.TMR3IF = 0U;
            PIE2bits.TMR3IE = 1U;
            T3CONbits.TMR3ON = 1U;
            break;
        default: return false;
    }
    halEnableInterrupts(priority);
    return true;
}

/**
 * @brief Acknowledge a pending interrupt on an enabled timer.
 * @param timer The same timer passed to timerAttachInterrupt().
 * @return True if the timer interrupt was pending and its flag was cleared.
 *
 * Call from the timer's interrupt handler. Timers 0, 1, and 3 reload the count
 * here, so interrupt latency adds to their period. Timer2 resets its count in hardware.
 */
static inline bool timerInterruptFired(Timer timer) {
    switch (timer) {
        case TIMER0:
            if (!INTCONbits.TMR0IE || !INTCONbits.TMR0IF) return false;
            TMR0H = (uint8_t)(halTimer0Reload >> 8);
            TMR0L = (uint8_t)halTimer0Reload;
            INTCONbits.TMR0IF = 0U;
            return true;
        case TIMER1:
            if (!PIE1bits.TMR1IE || !PIR1bits.TMR1IF) return false;
            TMR1H = (uint8_t)(halTimer1Reload >> 8);
            TMR1L = (uint8_t)halTimer1Reload;
            PIR1bits.TMR1IF = 0U;
            return true;
        case TIMER2:
            if (!PIE1bits.TMR2IE || !PIR1bits.TMR2IF) return false;
            PIR1bits.TMR2IF = 0U;
            return true;
        case TIMER3:
            if (!PIE2bits.TMR3IE || !PIR2bits.TMR3IF) return false;
            TMR3H = (uint8_t)(halTimer3Reload >> 8);
            TMR3L = (uint8_t)halTimer3Reload;
            PIR2bits.TMR3IF = 0U;
            return true;
        default: return false;
    }
}

#endif /* PIC18F4520_HAL_H */
