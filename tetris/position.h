#pragma once

#include <stdint.h>

typedef struct Position {
    uint8_t x;
    uint8_t y;
} Position;

Position position_sum(
    Position a,
    Position b
);

Position position_rotate(
    Position position,
    uint8_t rotation,
    uint8_t moving_center
);

