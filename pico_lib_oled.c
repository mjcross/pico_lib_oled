#include <stdio.h>
#include "pico/stdlib.h"
#include "lib_oled/oled.h"

int main() {
    // NB: We aren't enabling the UART or USB, so stdout will appear only on the OLED.
    //     To copy stdout to the other devices call stdio_init_all() and enable the
    //     appropriate driver in CMakeLists.txt.

    oled_init();

    // display some text
    set_cursor_pos(0, 2);
    printf("Hello, World\n");

    // show a moving 'snake'
    int head_x = NUM_Y_PIXELS - 1, head_y = NUM_Y_PIXELS - 1, head_dx = 1, head_dy = 1;
    int tail_x = FONT_BYTES_PER_CODE + 1, tail_y = FONT_BYTES_PER_CODE + 1, tail_dx = 1, tail_dy = 1;
    while(true) {
        set_pixel(head_x, head_y);
        clear_pixel(tail_x, tail_y);
        if (head_x + head_dx < 0 || head_x + head_dx >= NUM_X_PIXELS) {
            head_dx = -head_dx;
        }
        head_x += head_dx;
        if (head_y + head_dy <= FONT_BYTES_PER_CODE || head_y + head_dy >= NUM_Y_PIXELS) {
            head_dy = -head_dy;
        }
        head_y += head_dy;
        if (tail_x + tail_dx < 0 || tail_x + tail_dx >= NUM_X_PIXELS) {
            tail_dx = -tail_dx;
        }
        tail_x += tail_dx;
        if (tail_y + tail_dy <= FONT_BYTES_PER_CODE || tail_y + tail_dy >= NUM_Y_PIXELS) {
            tail_dy = -tail_dy;
        }
        tail_y += tail_dy;
        sleep_ms(5);
    }
}
