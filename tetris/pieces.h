#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "../screen.h"
#include "position.h"
#include "board.h"
#include "colours.h"

#define DEFAULT_ROTATION 0

typedef struct Piece {
    uint8_t size;
    uint8_t moving_center;
    Position definition[4];
} Piece;


int calculate_rotation_x_shift(
    Position position,
    uint8_t rotation,
    Piece piece
);

bool can_place_piece(
    uint8_t board[SCREEN_X][SCREEN_Y],
    Position position,
    uint8_t rotation,
    Piece piece
);

typedef struct PieceDrawDef {
    Piece piece;
    Position position;
    uint8_t rotation;
    enum Colour colour;
} PieceDrawDef;

Piece get_random_piece();

