#include <time.h>

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

void copy_board(
    char *screen,
    char *board
) {
    for (int x = 0; x < BOARD_SIZE_X; ++x) {
        for (int y = 0; y < BOARD_SIZE_Y; ++y) {
            *(screen + x * SCREEN_Y + y) = *(board + x * SCREEN_Y + y);
        }
    }
}

typedef struct Position {
    char x;
    char y;
} Position;

typedef struct Piece {
    char size;
    char moving_center;
    Position definition[4];
} Piece;

#define NUMBER_OF_COLOURS 7
enum Colour colours[NUMBER_OF_COLOURS] = {COLOUR_RED, COLOUR_ORANGE, COLOUR_YELLOW, COLOUR_GREEN, COLOUR_BLUE, COLOUR_DEEP_BLUE, COLOUR_VIOLET}; 

#define NUMBER_OF_PIECES 7

static const char default_rotation = 0;

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
    char rotation,
    char moving_center
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
    char rotation,
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

char can_place_piece(
    char *screen,
    Position position,
    char rotation,
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
        if (final.y < 0) {
            return true;
        }
        if (*(screen + final.x * BOARD_SIZE_Y + final.y)) {
            return false;
        }
    }
    return true;
}

#define PREDICTION_SIZE 6

void erase_prediction(
    char *board,
    char *screen
) {
    for (int x = BOARD_SIZE_X + 1; x < SCREEN_X; ++x) {
        for (int y = 0; y < PREDICTION_SIZE; ++y) {
            *(screen + x * SCREEN_Y + y) = *(board + x * SCREEN_Y + y) = 0;
        }
    }
}

void draw_piece(
    char *board,
    char *screen,
    Position position,
    char rotation,
    Piece piece,
    enum Colour colour
) {
    copy_board((char *) screen, (char *) board);
    for (int i = 0; i < piece.size; ++i) {
        Position rotated = position_rotate(
            *(piece.definition + i),
            rotation,
            piece.moving_center
        );

        Position final = position_sum(position, rotated);
        if (final.y >= 0) {
            char* screen_colour = screen + final.x * BOARD_SIZE_Y + final.y;
            if (*screen_colour == 0) {
                *screen_colour = colour;
            }
        }
    }
    renderer_render((char *) screen);
}

char place_piece(
    char *board,
    char *screen,
    Position position,
    char rotation,
    Piece piece,
    enum Colour colour
) {
    if (can_place_piece(board, position, rotation, piece)) {
        draw_piece(board, screen, position, rotation, piece, colour);
        return true;
    }
    return false;
}

typedef struct PieceDrawDef {
    Piece piece;
    Position position;
    char rotation;
    enum Colour colour;
} PieceDrawDef;

Position prediction_position = {
    .x = 13,
    .y = 2
};
PieceDrawDef next_piece;
PieceDrawDef select_next_piece(
    char *screen,
    char *board
) {
    PieceDrawDef result_piece = next_piece;
    PieceDrawDef new_piece = {
        pieces[rand() % NUMBER_OF_PIECES],
        {START_X, 0},
        default_rotation,
        colours[rand() % NUMBER_OF_COLOURS]
    };
    next_piece = new_piece;
    erase_prediction((char *) board, (char *) screen);
    draw_piece((char *) board, (char *) screen, prediction_position, default_rotation, next_piece.piece, next_piece.colour);
    return result_piece;
}

void game_over(
    char *screen,
    char *board
) {
    for (int y = 0; y < BOARD_SIZE_Y; ++y) {
        for (int x = 0; x < BOARD_SIZE_X; ++x) {
            *(screen + x * SCREEN_Y + y) = COLOUR_RED;
        }

        renderer_render((char *) screen);
        renderer_delay(10);
    }

    for (int y = SCREEN_Y - 1; y >= 0; --y) {
        for (int x = 0; x < BOARD_SIZE_X; ++x) {
            *(screen + x * SCREEN_Y + y) = 0;
            *(board + x * SCREEN_Y + y) = 0;
        }

        renderer_render((char *) screen);
        renderer_delay(10);
    }
}

void check_board(
    char *board,
    char *screen
) {
    int by = BOARD_SIZE_Y - 1;
    for (int y = BOARD_SIZE_Y - 1; y > 0; --y) {
        char row_is_full = true;
        for (int x = 0; x < BOARD_SIZE_X; ++x) {
            row_is_full = row_is_full && *(board + x * SCREEN_Y + y);
            *(board + x * SCREEN_Y + by) = *(board + x * SCREEN_Y + y);
        }
        if (!row_is_full) {
            --by;
        } else {
            renderer_play_sound(TETRIS_SOUND_CLEAR_ROW);
            renderer_delay(TETRIS_SOUND_CLEAR_ROW.length / 16);
        }
    }
    copy_board((char *) screen, (char *) board);
    renderer_render((char *) screen);
}

static int game_time = 0;
static char running = true;

int piece_down(
    char *board,
    char *screen,
    PieceDrawDef *piece
) {
    Position new_position = {
        .x = piece->position.x,
        .y = piece->position.y + 1
    };
    if (place_piece((char *) board, (char *) screen, new_position, piece->rotation, piece->piece, piece->colour)) {
        piece->position = new_position;
        return 1;
    } else {
        copy_board((char *) board, (char *) screen);
        check_board((char *) screen, (char *) board);
        PieceDrawDef next_piece = select_next_piece((char *) screen, (char *) board);
        piece->piece = next_piece.piece;
        piece->rotation = next_piece.rotation;
        piece->position = next_piece.position;
        piece->colour = next_piece.colour;
        if (!place_piece((char *) board, (char *) screen, piece->position, piece->rotation, piece->piece, piece->colour)) {
            game_over((char *) screen, (char *) board);
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    srand(time(NULL));
    
    renderer_init();

    char board[SCREEN_X][SCREEN_Y];
    char screen[SCREEN_X][SCREEN_Y];

    for (int x = 0; x < SCREEN_X; ++x) {
        for (int y = 0; y < SCREEN_Y; ++y) {
	        board[x][y] = false;
	        screen[x][y] = false;
	    }
    }

    for (int y = 0; y < SCREEN_Y; ++y) {
        screen[BOARD_SIZE_X][y] = COLOUR_WALL;
    }

    for (int x = BOARD_SIZE_X; x < SCREEN_X; ++x) {
        screen[x][PREDICTION_SIZE] = COLOUR_WALL;
    }

    select_next_piece((char *) screen, (char *) board);
    PieceDrawDef piece = select_next_piece((char *) screen, (char *) board);
    while (running) {
        enum Event event = EVENT_EMPTY;
        do {
            event = renderer_get_event();
            switch(event) {
                case EVENT_EXIT: {
                    running = false;
                    break;
                }
                case EVENT_LEFT: {
                    Position new_position = {
                        .x = piece.position.x - 1,
                        .y = piece.position.y
                    };
                    if (place_piece((char *) board, (char *) screen, new_position, piece.rotation, piece.piece, piece.colour)) {
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
                    if (place_piece((char *) board, (char *) screen, new_position, piece.rotation, piece.piece, piece.colour)) {
                        piece.position = new_position;
                        renderer_play_sound(TETRIS_SOUND_MOVE);
                    }
                    break;
                }
                case EVENT_DOWN: {
                    if (piece_down((char *) board, (char *) screen, &piece)) {
                        renderer_play_sound(TETRIS_SOUND_MOVE);
                    }
                    break;
                }
                case EVENT_UP: {
                    while (piece_down((char *) board, (char *) screen, &piece)) { renderer_delay(12); } 
                    renderer_play_sound(TETRIS_SOUND_PLACE);
                    renderer_delay(200);
                    break;
                }
                case EVENT_SPACE: {
                    char next_rotation = piece.rotation + 1;
                    if (next_rotation > 3) next_rotation = 0;
                    char shift = calculate_rotation_x_shift(
                        piece.position,
                        next_rotation,
                        piece.piece
                    );
                    Position new_position = {
                        .x = piece.position.x + shift,
                        .y = piece.position.y
                    };
                    if (place_piece((char *) board, (char *) screen, new_position, next_rotation, piece.piece, piece.colour)) {
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

        if (game_time == 300) {
            game_time = 0;
            piece_down((char *) board, (char *) screen, &piece);
        }

        game_time += 10;
        renderer_delay(10);

    }
    renderer_destroy();

    return 0;
}
