#include "board.h"
#include "extflash.h"
#include "lcd.h"
#include "link.h"
#include "ui.h"

/* Also gives the panel time to finish going to sleep */
#define SLEEP_QUIET_MS 100

int main(void) {
    board_init();
    link_init();
    extflash_init();
    lcd_init();
    ui_init();

    for (;;) {
        link_poll();
        ui_task();
        board_set_fast(ui_needs_speed());

        if (ui_can_sleep() && link_quiet(SLEEP_QUIET_MS) && board_deep_sleep()) {
            link_on_wake();
        }
    }
}
