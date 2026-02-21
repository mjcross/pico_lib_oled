#pragma once

#include "pico/stdlib.h"
#include "pico/stdio/driver.h"
#include "font.h"

// dimensions of the display panel
#define NUM_X_PIXELS        128
#define NUM_Y_PIXELS        64
#define PIXELS_PER_BYTE     8
#define TABSTOPS            4

// how often we want to refresh the display from the frame buffer
#define FRAME_PERIOD_MS     20      // 50 Hz

// clock rate for the SPI interface
// the ssd1309 is specified up to 10 Mbit/sec
#define DISPLAY_SPI_BITRATE 10 * 1000 * 1000

// pins to use for the SPI interface
// we will use the spi0 peripheral and the following GPIO pins (see the
// GPIO function select table in the Pico datasheet).
#define SPI_DEVICE          spi0
#define PIN_CS              17      // chip select (active low)
#define PIN_SCK             18      // SPI clock
#define PIN_MOSI            19      // SPI data transmit (MOSI)
#define PIN_DC              20      // data/command mode (low for command)
#define PIN_R               21      // reset (active low)

// public API
void oled_init();
void set_pixel(uint x, uint y);
void clear_pixel(uint x, uint y);
void draw_line(int x0, int y0, int x1, int y1);
void set_cursor_pos(uint text_row, uint text_col);
void clear_screen();