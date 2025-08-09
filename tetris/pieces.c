#include <stdbool.h>
#include <stdlib.h>

#include "pieces.h"
#include "position.h"
#include "board.h"
#include "colours.h"

#define NUMBER_OF_PIECES 7
Piece pieces[NUMBER_OF_PIECES] = {
    {4, true, {{0, -1}, {0, 0}, {0, 1}, {0, 2}}},
    {4, false, {{0, -1}, {0, 0}, {0, 1}, {-1, 1}}},
    {4, false, {{0, -1}, {0, 0}, {0, 1}, {1, 1}}},
    {4, true, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}},
    {4, false, {{0, 0}, {1, 0}, {0, 1}, {-1, 1}}},
    {4, false, {{0, 0}, {1, 0}, {-1, 0}, {0, 1}}},
    {4, false, {{0, 0}, {-1, 0}, {0, 1}, {1, 1}}}
};

int calculate_rotation_x_shift(
    Position position,
    uint8_t rotation,
    Piece piece
) {
    int shift = 0;
    for (int i = 0; i < piece.size; ++i) {
        Position rotated = position_rotate(
            *(piece.definition + i),
            rotation,
            piece.moving_center
        );

        Position final = position_sum(rotated, position);
        if (final.x >= BOARD_SIZE_X) {
            int new_shift = BOARD_SIZE_X - final.x - 1;
            if (new_shift < shift) shift = new_shift;
        }
        if (final.x < 0) {
            int new_shift = -final.x;
            if (new_shift > shift) shift = new_shift;
        }
    }
    return shift;
}

bool can_place_piece(
    uint8_t board[SCREEN_X][SCREEN_Y],
    Position position,
    uint8_t rotation,
    Piece piece
) {
    for (int i = 0; i < piece.size; ++i) {
        Position rotated = position_rotate(
            *(piece.definition + i),
            rotation,
            piece.moving_center
        );

        Position final = position_sum(rotated, position);
        if (final.x >= BOARD_SIZE_X) {
            return false;
        }
        if (final.x < 0) {
            return false;
        }
        if (final.y >= BOARD_SIZE_Y) {
            return false;
        }
        if (board[final.x][final.y]) {
            return false;
        }
    }
    return true;
}

Piece get_random_piece() {
    return pieces[rand() % NUMBER_OF_PIECES];
}

