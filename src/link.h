#pragma once

#include <stdbool.h>
#include <stdint.h>

void link_init(void);

/* Handles received frames, calling the ui_on_* handlers */
void link_poll(void);

/* Nothing is arriving or waiting to be handled */
bool link_rx_idle(void);

/* Same, and nothing has arrived for this long */
bool link_quiet(uint32_t ms);

/* Discards whatever deep sleep cut off on the line */
void link_on_wake(void);
