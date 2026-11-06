#pragma once

#include <stdbool.h>
#include <stdint.h>

#define KEY_ROWS 5
#define KEY_COLS 16

/* Physical position of a matrix key on its row, in quarter key units; width 0 means no key */
typedef struct {
    uint8_t x;
    uint8_t width;
} key_rect_t;

static const key_rect_t key_rects[KEY_ROWS][KEY_COLS] = {
    [0] = {
        {0, 4}, {4, 4}, {8, 4}, {12, 4}, {16, 4}, {20, 4}, {24, 4}, {28, 4},
        {32, 4}, {36, 4}, {40, 4}, {44, 4}, {48, 4}, {52, 8}, {60, 4}, {60, 4},
    },
    [1] = {
        {0, 6}, {6, 4}, {10, 4}, {14, 4}, {18, 4}, {22, 4}, {26, 4}, {30, 4},
        {34, 4}, {38, 4}, {42, 4}, {46, 4}, {50, 4}, {54, 6}, {0, 0}, {60, 4},
    },
    [2] = {
        {0, 7}, {7, 4}, {11, 4}, {15, 4}, {19, 4}, {23, 4}, {27, 4}, {31, 4},
        {35, 4}, {39, 4}, {43, 4}, {47, 4}, {0, 0}, {51, 9}, {0, 0}, {60, 4},
    },
    [3] = {
        {0, 9}, {9, 4}, {13, 4}, {17, 4}, {21, 4}, {25, 4}, {29, 4}, {33, 4},
        {37, 4}, {41, 4}, {45, 4}, {0, 9}, {0, 0}, {49, 7}, {56, 4}, {60, 4},
    },
    /* Columns 3, 5 and 7 are the same bar: boards wire one of them as the spacebar */
    [4] = {
        {0, 5}, {5, 5}, {10, 5}, {15, 25}, {0, 0}, {15, 25}, {0, 0}, {15, 25},
        {0, 0}, {0, 0}, {40, 5}, {45, 5}, {0, 0}, {52, 4}, {56, 4}, {60, 4},
    },
};

static inline key_rect_t key_rect(uint8_t row, uint8_t col, bool iso) {
    if (iso) {
        if (row == 2 && col == 12) return (key_rect_t){51, 4};
        if (row == 2 && col == 13) return (key_rect_t){55, 5};
        if (row == 3 && col == 0) return (key_rect_t){0, 5};
        if (row == 3 && col == 11) return (key_rect_t){5, 4};
    }
    return key_rects[row][col];
}
