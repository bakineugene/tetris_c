#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define true 1
#define false 0

#define SCREEN_X 16
#define SCREEN_Y 24

#define BOARD_SIZE_X 10 
#define BOARD_SIZE_Y 24 

#define START_X 4

#include "colours.h"

#if defined(__AVR__)
#include <avr/pgmspace.h>
#include "avr/renderer.h"
#else
#include <stdlib.h>
#include "sdl2/renderer.h"
#endif

#define TETRIS_SOUND_CLEAR_ROW SOUND_DZIN
#define TETRIS_SOUND_MOVE SOUND_SPOON2
#define TETRIS_SOUND_PLACE SOUND_CLICK
#define TETRIS_SOUND_TURN SOUND_SPOON

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
    PieceDrawDef next_piece;
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
            game->screen[x][y] = game->board[x][y] = 0;
        }
    }
}

void draw_piece(
    Tetris* game,
    Position position,
    uint8_t rotation,
    Piece piece,
    enum Colour colour
) {
    copy_board_to_screen(game);
    for (int i = 0; i < piece.size; ++i) {
        Position rotated = position_rotate(
            *(piece.definition + i),
            rotation,
            piece.moving_center
        );

        Position final = position_sum(position, rotated);
        if (final.y >= 0) {
            if (game->screen[final.x][final.y] == 0) {
                game->screen[final.x][final.y] = colour;
            }
        }
    }
    renderer_render((uint8_t *) game->screen);
}

uint8_t place_piece(
    Tetris* game,
    Position position,
    uint8_t rotation,
    Piece piece,
    enum Colour colour
) {
    if (can_place_piece(game, position, rotation, piece)) {
        draw_piece(game, position, rotation, piece, colour);
        return true;
    }
    return false;
}

PieceDrawDef select_next_piece(Tetris *game) {
    PieceDrawDef result_piece = game->next_piece;
    PieceDrawDef new_piece = {
        pieces[rand() % NUMBER_OF_PIECES],
        {START_X, 1},
        DEFAULT_ROTATION,
        colours[rand() % NUMBER_OF_COLOURS]
    };
    game->next_piece = new_piece;
    erase_prediction(game);
    draw_piece(
        game,
        (Position) { .x = 13, .y = 2 },
        DEFAULT_ROTATION,
        game->next_piece.piece,
        game->next_piece.colour
    );
    return result_piece;
}

void game_over(Tetris* game) {
    for (int y = 0; y < BOARD_SIZE_Y; ++y) {
        for (int x = 0; x < BOARD_SIZE_X; ++x) {
            game->screen[x][y] = COLOUR_RED;
        }

        renderer_render((uint8_t *) game->screen);
        renderer_delay(10);
    }

    for (int y = SCREEN_Y - 1; y >= 0; --y) {
        for (int x = 0; x < BOARD_SIZE_X; ++x) {
            game->screen[x][y] = game->board[x][y] = 0;
        }

        renderer_render((uint8_t *) game->screen);
        renderer_delay(10);
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
            renderer_play_sound(TETRIS_SOUND_CLEAR_ROW);
            renderer_delay(TETRIS_SOUND_CLEAR_ROW.length / 16);
        }
    }
    copy_board_to_screen(game);
    renderer_render((uint8_t *) game->screen);
}

int piece_down(
    Tetris* game,
    PieceDrawDef *piece
) {
    Position new_position = {
        .x = piece->position.x,
        .y = piece->position.y + 1
    };
    if (place_piece(game, new_position, piece->rotation, piece->piece, piece->colour)) {
        piece->position = new_position;
        return 1;
    } else {
        copy_screen_to_board(game);
        check_board(game);
        PieceDrawDef next_piece = select_next_piece(game);
        piece->piece = next_piece.piece;
        piece->rotation = next_piece.rotation;
        piece->position = next_piece.position;
        piece->colour = next_piece.colour;
        if (!place_piece(game, piece->position, piece->rotation, piece->piece, piece->colour)) {
            game_over(game);
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    srand(time(NULL));
    renderer_init();

    Tetris game;
    game.time = 0;
    game.running = true;

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
    PieceDrawDef piece = select_next_piece(&game);
    while (game.running) {
        enum Event event = EVENT_EMPTY;
        do {
            event = renderer_get_event();
            switch(event) {
                case EVENT_EXIT: {
                    game.running = false;
                    break;
                }
                case EVENT_LEFT: {
                    Position new_position = {
                        .x = piece.position.x - 1,
                        .y = piece.position.y
                    };
                    if (place_piece(&game, new_position, piece.rotation, piece.piece, piece.colour)) {
                        piece.position = new_position;
                        renderer_play_sound(TETRIS_SOUND_MOVE);
                    }
                    break;
                }
                case EVENT_RIGHT: {
                    Position new_position = {
                        .x = piece.position.x + 1,
                        .y = piece.position.y
                    };
                    if (place_piece(&game, new_position, piece.rotation, piece.piece, piece.colour)) {
                        piece.position = new_position;
                        renderer_play_sound(TETRIS_SOUND_MOVE);
                    }
                    break;
                }
                case EVENT_DOWN: {
                    if (piece_down(&game, &piece)) {
                        renderer_play_sound(TETRIS_SOUND_MOVE);
                    }
                    break;
                }
                case EVENT_UP: {
                    while (piece_down(&game, &piece)) { renderer_delay(12); } 
                    renderer_play_sound(TETRIS_SOUND_PLACE);
                    renderer_delay(200);
                    break;
                }
                case EVENT_SPACE: {
                    uint8_t next_rotation = piece.rotation + 1;
                    if (next_rotation > 3) next_rotation = 0;
                    uint8_t shift = calculate_rotation_x_shift(
                        piece.position,
                        next_rotation,
                        piece.piece
                    );
                    Position new_position = {
                        .x = piece.position.x + shift,
                        .y = piece.position.y
                    };
                    if (place_piece(&game, new_position, next_rotation, piece.piece, piece.colour)) {
                        piece.rotation = next_rotation;
                        piece.position = new_position;
                        renderer_play_sound(TETRIS_SOUND_TURN);
                        renderer_delay(200);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        } while (event != EVENT_EMPTY);

        if (game.time == 300) {
            game.time = 0;
            piece_down(&game, &piece);
        }

        game.time += 10;
        renderer_delay(10);

    }
    renderer_destroy();

    return 0;
}
