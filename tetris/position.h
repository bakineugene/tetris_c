#pragma once

#include <stdint.h>

typedef struct Position {
    uint8_t x;
    uint8_t y;
} Position;

Position position_sum(
    Position a,
    Position b
) {
    Position result = {
        .x = a.x + b.x,
        .y = a.y + b.y
    };
    return result;
}

Position position_rotate(
    Position position,
    uint8_t rotation,
    uint8_t moving_center
) {
    Position result;

    switch(rotation) {
        case 0: {
            result.x = position.x;  
            result.y = position.y;
            break;
        }
        case 1: {
            result.x = -position.y;  
            result.y = position.x;
            if (moving_center) {
                result.x += 1;
            }
            break;
        }
        case 2: {
            result.x = -position.x;  
            result.y = -position.y;
            if (moving_center) {
                result.x += 1;
                result.y += 1;
            }
            break;
        }
        case 3: {
            result.x = position.y;  
            result.y = -position.x;
            if (moving_center) {
                result.y += 1;
            }
            break;
        }
    }

    return result;
}

