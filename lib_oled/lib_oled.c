/**
 * Copyright (c) 2026 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "oled.h"

// modes for the ssd1309 data/command pin (see the datasheet)
#define DC_COMMAND_MODE     0
#define DC_DATA_MODE        1

// module variables
static uint8_t frame_buffer[ NUM_X_PIXELS * NUM_Y_PIXELS / PIXELS_PER_BYTE ];
static int dma_ch_transfer_fb;
static volatile bool display_needs_refresh = false;
static volatile uint fb_cursor_index = 0;
static struct repeating_timer frame_timer;
static void fb_out_chars(const char *buf, int len);
static stdio_driver_t fb_stdio_driver = { fb_out_chars };


// configure a dma channel to send the frame buffer over SPI
static void dma_init() {
    dma_ch_transfer_fb = dma_claim_unused_channel(true);
    dma_channel_config_t c = dma_channel_get_default_config(dma_ch_transfer_fb);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(SPI_DEVICE, true));
    dma_channel_configure(
        dma_ch_transfer_fb,
        &c,                             // the channel_config above
        &spi_get_hw(SPI_DEVICE)->dr,    // write address (doesn't increment)
        frame_buffer,                   // initial read address
        dma_encode_transfer_count(count_of(frame_buffer)),
        false                           // don't trigger yet
    );
}

// initialise the SPI interface
static void interface_init() {
    // configure the SPI controller for 8-bit transfers and Motorola SPI mode 0
    spi_init(SPI_DEVICE, DISPLAY_SPI_BITRATE);
    spi_set_format(SPI_DEVICE, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // configure our interface pins
    gpio_set_function(PIN_CS,     GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,    GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI,   GPIO_FUNC_SPI);
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_R);
    gpio_set_dir(PIN_R, GPIO_OUT);
}

// reset and initialise the display
static void display_reset() {
    // send active-low reset pulse
    gpio_put(PIN_R, 0);
    sleep_ms(1);
    gpio_put(PIN_R, 1);
    sleep_ms(1);

    // wake up the display and set horizontal addressing mode
    gpio_put(PIN_DC, DC_COMMAND_MODE); 
    uint8_t cmd_list[] = { 0xaf, 0x20, 0x00 };
    spi_write_blocking(SPI_DEVICE, cmd_list, sizeof(cmd_list));
    gpio_put(PIN_DC, DC_DATA_MODE);

    clear_screen();
}


// a simple stdout driver for the display
static void fb_out_chars(const char *buf, int len) {
    uint font_index;
    while (len) {
        char code = *buf;
        buf += 1;
        len -= 1;
        while (fb_cursor_index >= sizeof(frame_buffer)) {
            // scroll frame buffer (on RP2350 you could use a decementing dma transfer but it's probably overkill)
            memmove(frame_buffer, frame_buffer + NUM_X_PIXELS, sizeof(frame_buffer) - NUM_X_PIXELS);
            fb_cursor_index -= NUM_X_PIXELS;
            memset(&frame_buffer[sizeof(frame_buffer) - NUM_X_PIXELS - 1], 0x00, NUM_X_PIXELS);
        }
        // handle control codes
        if (code == '\n') {
            fb_cursor_index = (fb_cursor_index / NUM_X_PIXELS + 1) * NUM_X_PIXELS;
        } else if (code == '\t') {
            fb_cursor_index = (fb_cursor_index / (TABSTOPS * FONT_BYTES_PER_CODE) + 1) * TABSTOPS * FONT_BYTES_PER_CODE;
        } else {
            // handle alphanumeric codes
            if (code < FONT_CODE_FIRST || code > FONT_CODE_LAST ) {
                font_index = FONT_INDEX_UNDEF;
            } else {
                font_index = FONT_BYTES_PER_CODE * (FONT_INDEX_START + code - FONT_CODE_FIRST);
            }
            // copy bitmap to frame buffer and advance cursor
            memcpy(&frame_buffer[fb_cursor_index], &font[font_index], FONT_BYTES_PER_CODE);
            fb_cursor_index += FONT_BYTES_PER_CODE;
        }
    }
    display_needs_refresh = true;
}


// some simple graphics funtions (for the display memory layout see the datasheet)
void set_pixel(uint x, uint y) {
    if (x < NUM_X_PIXELS && y < NUM_Y_PIXELS) {
        frame_buffer[x + (y / 8) * NUM_X_PIXELS] |= (1 << (y % 8));
        display_needs_refresh = true;
    }
}

void clear_pixel(uint x, uint y) {
    if (x < NUM_X_PIXELS && y < NUM_Y_PIXELS) {
        frame_buffer[x + (y / 8) * NUM_X_PIXELS] &= ~(1 << (y % 8));
        display_needs_refresh = true;
    }
}

void draw_line(int x0, int y0, int x1, int y1) {
  int dx =  abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs (y1 - y0), sy = y0 < y1 ? 1 : -1; 
  int err = dx + dy, e2;
  while(true) {
    set_pixel (x0, y0);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
  display_needs_refresh = true;
}

void clear_screen() {
    // zero the frame buffer and flag it to be transferred
    bzero(frame_buffer, sizeof(frame_buffer));
    fb_cursor_index = 0;
    display_needs_refresh = true;
}

// set the text output position
// rows go from 0 at the top to NUM_Y_PIXELS/8 - 1 and columns go from 0 on the left to NUM_X_PIXELS/8 - 1
void set_cursor_pos(uint text_row, uint text_col) {
    if (text_row < NUM_Y_PIXELS / PIXELS_PER_BYTE && text_col < NUM_X_PIXELS / FONT_BYTES_PER_CODE) {
        fb_cursor_index = text_row * NUM_X_PIXELS + text_col * FONT_BYTES_PER_CODE;
    }
}

// the callback function for our frame-rate timer
static bool frame_refresh_callback(__unused struct repeating_timer *t) {
    if (display_needs_refresh) {
        dma_channel_set_read_addr(dma_ch_transfer_fb, frame_buffer, true);  // reset and trigger the dma channel
        display_needs_refresh = false;
    }
    return true;    // reschedule the timer
}

// initialise the library
void oled_init() {
    dma_init();
    interface_init();
    display_reset();

    // install our simple driver to copy stdout to the frame buffer
    stdio_set_driver_enabled(&fb_stdio_driver, true);

    // start the frame refresh
    add_repeating_timer_ms(FRAME_PERIOD_MS, frame_refresh_callback, NULL, &frame_timer);
}
