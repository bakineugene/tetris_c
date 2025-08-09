#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "tetris.h"

#include "../build/sounds/dzin.h"
#include "../build/sounds/bump.h"
#include "../build/sounds/spoon.h"
#include "../build/sounds/spoon2.h"
#include "../build/sounds/click.h"

#include "../game.h"
#include "../renderer.h"
#include "../screen.h"
#include "../sounds.h"
#include "../events.h"

#include "position.h"
#include "colours.h"
#include "pieces.h"
#include "board.h"

#define START_X 4
#define START_Y 1

#define TETRIS_SOUND_CLEAR_ROW SOUND_DZIN
#define TETRIS_SOUND_MOVE SOUND_SPOON2
#define TETRIS_SOUND_PLACE SOUND_CLICK
#define TETRIS_SOUND_TURN SOUND_SPOON

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
        get_random_colour()
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
    if (can_place_piece(game->board, new_position, new_rotation, game->piece.piece)) {
        game->piece.position = new_position;
        game->piece.rotation = new_rotation;
        draw_piece(game, game->piece);
        return true;
    }
    return false;
}

bool piece_change_position(Tetris* game, Position increment) {
    Position new_position = position_sum(game->piece.position, increment);
    if (can_place_piece(game->board, new_position, game->piece.rotation, game->piece.piece)) {
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

void game_start_internal(Tetris* game) {
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

void game_start(Renderer renderer) {
    Tetris tetris = new_tetris(renderer);
    game_start_internal(&tetris);
}

