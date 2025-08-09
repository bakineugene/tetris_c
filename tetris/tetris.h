#pragma once

#include "../renderer.h"
#include "../screen.h"
#include "pieces.h"

typedef struct Tetris {
    int time;
    bool running;
    Renderer renderer;
    PieceDrawDef next_piece;
    PieceDrawDef piece;
    uint8_t board[SCREEN_X][SCREEN_Y];
    uint8_t screen[SCREEN_X][SCREEN_Y];
} Tetris;

void game_start(Renderer renderer);

