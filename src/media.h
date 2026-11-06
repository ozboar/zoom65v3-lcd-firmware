#pragma once

#include <stdbool.h>
#include <stdint.h>

/* The stock upload sequence, stored where the stock firmware keeps uploads */
bool     media_upload_start(uint8_t channel);
bool     media_upload_length(uint32_t length);
bool     media_upload_end(void);
bool     media_delete(uint8_t command);
bool     media_upload_active(void);
bool     media_upload_receiving(void);
uint16_t media_next_index(void);
uint8_t  media_packet_length(void);
bool     media_on_packet(const uint8_t *packet, uint8_t length);
void     media_upload_cancel(void);
void     media_task(void);

/* The image and GIF page, using uploads or else the stock defaults */
void media_page_enter(bool show_gif);

/* A paused GIF holds its current frame. The first frame always shows */
void media_page_task(bool playing);

/* True once after the page draws, so anything on top of it can be drawn again */
bool media_page_take_drawn(void);

/* Draws the current picture again, without moving the GIF on */
void media_page_redraw(void);
