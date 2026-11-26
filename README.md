# Microprocessor

This repository contains microprocessor course labs for the [PIC18F4520](https://www.microchip.com/en-us/product/pic18f4520). Labs 1–5 focus on assembly programming with the MPLAB XC8 PIC Assembler. Labs 6–10 use a mix of C and assembly to work with digital pins, interrupts, timers, servos, analog inputs, and serial communication (UART).

I wrote the [hardware abstraction layer (HAL)](lib/pic18f4520_hal.h) for PIC18F4520. It lets you write code in the style of an Arduino sketch.

The [PIC18F4520 HAL Guide](https://moon-jam.github.io/microprocessor/) covers wiring, function usage, and worked examples in Traditional Chinese. To read it offline, open [`docs/index.html`](docs/index.html) in a browser from a local checkout.

## Quick Start

The Makefile workflow uses MPLAB XC8 and the MPLAB Integrated Programming Environment (IPE) on macOS. Creating a project requires Python 3.9 or later. See the [build setup](docs/index.html#makefile) for the tool paths.

1. Run `make new` from the repository root and enter a project directory.
2. Run `make` in that directory to build `out/main.hex`.
3. Use `make flash` to program the chip and `make monitor` to open the serial console.

`make flash` supplies 5 V from the PICkit 3 by default. For a board with its own power supply, use `make flash TARGET_POWER=`. See the [PICkit 3 wiring and power settings](docs/index.html#pickit).

Pull requests and issues are welcome.
