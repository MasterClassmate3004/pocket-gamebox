# Pocket Gamebox firmware

Arduino sketch: [`PocketGamebox/PocketGamebox.ino`](PocketGamebox/PocketGamebox.ino)

## Hardware

This sketch targets the Raspberry Pi Pico (RP2040) and the 8-pin Waveshare 2.4-inch 240×320 SPI LCD with an ILI9341 controller. The pin assignments come from `PCB/Pocket_Gaming_Console.kicad_sch`:

| Function | Pico GPIO |
| --- | ---: |
| Up, Down, Left, Right | 1, 2, 3, 4 |
| A, B | 5, 6 |
| LCD CS, SCK, MOSI | 17, 18, 19 |
| LCD DC, RST, BL | 20, 21, 22 |

The buttons connect to GND and use internal pull-ups. The LCD power pin connects to the Pico's 3V3 pin; do not feed the LCD connector 5V.

## Install and upload

1. In Arduino IDE, install the **Raspberry Pi Pico/RP2040** board package by Earle F. Philhower III and select **Raspberry Pi Pico**.
2. Install **Adafruit ILI9341** and **Adafruit GFX Library** using Library Manager. Accept any dependency prompts.
3. Open `PocketGamebox/PocketGamebox.ino`, verify, and upload to the Pico over USB.

The splash screen appears on every power-up. The menu follows automatically, or press A to skip the splash.

## Controls

| Screen | Controls |
| --- | --- |
| Menu | Arrow keys change selection; A starts a game |
| Snake | Arrow keys turn; B returns home |
| Brick Breaker | Left/Right move the paddle; A launches the ball; B returns home |
| After a win or loss | A retries; B returns home |

The firmware has not been tested on the assembled hardware. If the screen stays blank, confirm that the purchased display is the 8-pin **ILI9341** version and that its connector pin order matches the schematic before changing the code or wiring.

Display reference: [Waveshare 2.4inch LCD Module](https://www.waveshare.com/product/displays/lcd-oled/2.4inch-lcd-module.htm).
