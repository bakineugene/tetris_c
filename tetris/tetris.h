#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../build/sounds/dzin.h"
#include "../build/sounds/bump.h"
#include "../build/sounds/spoon.h"
#include "../build/sounds/spoon2.h"
#include "../build/sounds/click.h"

#include "../renderer.h"
#include "../screen.h"
#include "../sounds.h"
#include "../events.h"
#include "../colours.h"

#define BOARD_SIZE_X 10 
#define BOARD_SIZE_Y 24 

#define START_X 4
#define START_Y 1

#define TETRIS_SOUND_CLEAR_ROW SOUND_DZIN
#define TETRIS_SOUND_MOVE SOUND_SPOON2
#define TETRIS_SOUND_PLACE SOUND_CLICK
#define TETRIS_SOUND_TURN SOUND_SPOON

#define DEFAULT_ROTATION 0

typedef struct Position {
    uint8_t x;
    uint8_t y;
} Position;

typedef struct Piece {
    uint8_t size;
    uint8_t moving_center;
    Position definition[4];
} Piece;

typedef struct PieceDrawDef {
    Piece piece;
    Position position;
    uint8_t rotation;
    enum Colour colour;
} PieceDrawDef;

typedef struct Tetris {
    int time;
    bool running;
    Renderer renderer;
    PieceDrawDef next_piece;
    PieceDrawDef piece;
    uint8_t board[SCREEN_X][SCREEN_Y];
    uint8_t screen[SCREEN_X][SCREEN_Y];
} Tetris;

void copy_board_to_screen(Tetris* game) {
    for (int x = 0; x < BOARD_SIZE_X; ++x) {
        for (int y = 0; y < BOARD_SIZE_Y; ++y) {
            game->screen[x][y] = game->board[x][y];
        }
    }
}

void copy_screen_to_board(Tetris* game) {
    for (int x = 0; x < BOARD_SIZE_X; ++x) {
        for (int y = 0; y < BOARD_SIZE_Y; ++y) {
            game->board[x][y] = game->screen[x][y];
        }
    }
}

#define NUMBER_OF_COLOURS 7
enum Colour colours[NUMBER_OF_COLOURS] = {COLOUR_RED, COLOUR_ORANGE, COLOUR_YELLOW, COLOUR_GREEN, COLOUR_BLUE, COLOUR_DEEP_BLUE, COLOUR_VIOLET}; 

#define NUMBER_OF_PIECES 7
#define DEFAULT_ROTATION 0

Piece pieces[NUMBER_OF_PIECES] = {
    {4, true, {{0, -1}, {0, 0}, {0, 1}, {0, 2}}},
    {4, false, {{0, -1}, {0, 0}, {0, 1}, {-1, 1}}},
    {4, false, {{0, -1}, {0, 0}, {0, 1}, {1, 1}}},
    {4, true, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}},
    {4, false, {{0, 0}, {1, 0}, {0, 1}, {-1, 1}}},
    {4, false, {{0, 0}, {1, 0}, {-1, 0}, {0, 1}}},
    {4, false, {{0, 0}, {-1, 0}, {0, 1}, {1, 1}}}
};

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
    Tetris* game,
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
        if (game->board[final.x][final.y]) {
            return false;
        }
    }
    return true;
}

#define PREDICTION_SIZE 6

void erase_prediction(Tetris* game) {
    for (int x = BOARD_SIZE_X + 1; x < SCREEN_X; ++x) {
        for (int y = 0; y < PREDICTION_SIZE; ++y) {
            game->screen[x][y] = 0;
        }
    }
}

void draw_piece(
    Tetris* game,
    PieceDrawDef draw_definition
) {
    copy_board_to_screen(game);
    for (int i = 0; i < draw_definition.piece.size; ++i) {
        Position rotated = position_rotate(
            *(draw_definition.piece.definition + i),
            draw_definition.rotation,
            draw_definition.piece.moving_center
        );

        Position final = position_sum(draw_definition.position, rotated);
        if (final.y >= 0) {
            if (game->screen[final.x][final.y] == 0) {
                game->screen[final.x][final.y] = draw_definition.colour;
            }
        }
    }
    game->renderer.render((uint8_t *) game->screen);
}

PieceDrawDef select_next_piece(Tetris *game) {
    PieceDrawDef result_piece = game->next_piece;
    PieceDrawDef new_piece = {
        pieces[rand() % NUMBER_OF_PIECES],
        { .x = 13, .y = 2 },
        DEFAULT_ROTATION,
        colours[rand() % NUMBER_OF_COLOURS]
    };
    game->next_piece = new_piece;
    erase_prediction(game);
    draw_piece(game, game->next_piece);

    result_piece.position = (Position) {.x = START_X, .y = START_Y};
    result_piece.rotation = DEFAULT_ROTATION;
    return game->piece = result_piece;
}

void game_over(Tetris* game) {
    for (int y = 0; y < BOARD_SIZE_Y; ++y) {
        for (int x = 0; x < BOARD_SIZE_X; ++x) {
            game->screen[x][y] = COLOUR_RED;
        }

        game->renderer.render((uint8_t *) game->screen);
        game->renderer.delay(10);
    }

    for (int y = SCREEN_Y - 1; y >= 0; --y) {
        for (int x = 0; x < BOARD_SIZE_X; ++x) {
            game->screen[x][y] = game->board[x][y] = 0;
        }

        game->renderer.render((uint8_t *) game->screen);
        game->renderer.delay(10);
    }
}

void check_board(Tetris* game) {
    int by = BOARD_SIZE_Y - 1;
    for (int y = BOARD_SIZE_Y - 1; y > 0; --y) {
        uint8_t row_is_full = true;
        for (int x = 0; x < BOARD_SIZE_X; ++x) {
            row_is_full = row_is_full && game->board[x][y];
            game->board[x][by] = game->board[x][y];
        }
        if (!row_is_full) {
            --by;
        } else {
            game->renderer.play_sound(TETRIS_SOUND_CLEAR_ROW);
            game->renderer.delay(TETRIS_SOUND_CLEAR_ROW.length / 16);
        }
    }
    copy_board_to_screen(game);
    game->renderer.render((uint8_t *) game->screen);
}

bool piece_change_rotation(Tetris* game, uint8_t increment) {
    uint8_t new_rotation = game->piece.rotation + increment;
    if (new_rotation > 3) new_rotation = 0;
    uint8_t shift = calculate_rotation_x_shift(
        game->piece.position,
        new_rotation,
        game->piece.piece
    );
    Position new_position = position_sum(game->piece.position, (Position) {.x = shift, .y = 0});
    if (can_place_piece(game, new_position, new_rotation, game->piece.piece)) {
        game->piece.position = new_position;
        game->piece.rotation = new_rotation;
        draw_piece(game, game->piece);
        return true;
    }
    return false;
}

bool piece_change_position(Tetris* game, Position increment) {
    Position new_position = position_sum(game->piece.position, increment);
    if (can_place_piece(game, new_position, game->piece.rotation, game->piece.piece)) {
        game->piece.position = new_position;
        draw_piece(game, game->piece);
        return true;
    }
    return false;
}

bool piece_down(Tetris* game) {
    if (piece_change_position(game, (Position) {.x = 0, .y = 1})) {
        return true;
    } else {
        copy_screen_to_board(game);
        check_board(game);
        select_next_piece(game);
        if (!piece_change_position(game, (Position) {.x = 0, .y = 0})) {
            game_over(game);
        }
    }
    return false;
}

void game_start(Tetris* game) {
    srand(time(NULL));
    game->running = true;
    while (game->running) {
        enum Event event = EVENT_EMPTY;
        do {
            event = game->renderer.get_event();
            switch(event) {
                case EVENT_EXIT: {
                    game->running = false;
                    break;
                }
                case EVENT_LEFT: {
                    if (piece_change_position(game, (Position) {.x = -1, .y = 0})) {
                        game->renderer.play_sound(TETRIS_SOUND_MOVE);
                    }
                    break;
                }
                case EVENT_RIGHT: {
                    if (piece_change_position(game, (Position) {.x = 1, .y = 0})) {
                        game->renderer.play_sound(TETRIS_SOUND_MOVE);
                    }
                    break;
                }
                case EVENT_DOWN: {
                    if (piece_down(game)) {
                        game->renderer.play_sound(TETRIS_SOUND_MOVE);
                    }
                    break;
                }
                case EVENT_UP: {
                    while (piece_down(game)) { game->renderer.delay(12); } 
                    game->renderer.play_sound(TETRIS_SOUND_PLACE);
                    game->renderer.delay(200);
                    break;
                }
                case EVENT_SPACE: {
                    if (piece_change_rotation(game, 1)) {
                        game->renderer.play_sound(TETRIS_SOUND_TURN);
                        game->renderer.delay(200);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        } while (event != EVENT_EMPTY);

        if (game->time == 300) {
            game->time = 0;
            piece_down(game);
        }

        game->time += 10;
        game->renderer.delay(10);
    }
}

Tetris new_tetris(Renderer renderer) {
    Tetris game;
    game.time = 0;
    game.renderer = renderer;

    for (int x = 0; x < SCREEN_X; ++x) {
        for (int y = 0; y < SCREEN_Y; ++y) {
	        game.board[x][y] = false;
	        game.screen[x][y] = false;
	    }
    }

    for (int y = 0; y < SCREEN_Y; ++y) {
        game.screen[BOARD_SIZE_X][y] = COLOUR_WALL;
    }

    for (int x = BOARD_SIZE_X; x < SCREEN_X; ++x) {
        game.screen[x][PREDICTION_SIZE] = COLOUR_WALL;
    }

    select_next_piece(&game);
    select_next_piece(&game);
    return game;
}
