#include <time.h>

#define true 1
#define false 0

#define SCREEN_X 16
#define SCREEN_Y 24

#define START_X 8

#include "colours.h"

#if defined(__AVR__)
#include <avr/pgmspace.h>
#include "avr_renderer.h"
#else
#include <stdlib.h>
#include "sdl_renderer.h"
#endif

void copy_board(
    char *screen,
    char *board
) {
    for (int x = 0; x < SCREEN_X; ++x) {
        for (int y = 0; y < SCREEN_Y; ++y) {
            *(screen + x * SCREEN_Y + y) = *(board + x * SCREEN_Y + y);
        }
    }
}

struct Position {
    char x;
    char y;
};

struct Piece {
    char size;
    char moving_center;
    struct Position definition[4];
};

static char pieces_number = 7;
static const char default_rotation = 0;

struct Piece pieces[7] = {
    {4, true, {{-1, 0}, {0, 0}, {1, 0}, {2, 0}}},
    {4, false, {{0, -1}, {0, 0}, {0, 1}, {-1, 1}}},
    {4, false, {{0, -1}, {0, 0}, {0, 1}, {1, 1}}},
    {4, true, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}},
    {4, false, {{0, 0}, {1, 0}, {0, 1}, {-1, 1}}},
    {4, false, {{0, 0}, {1, 0}, {-1, 0}, {0, 1}}},
    {4, false, {{0, 0}, {-1, 0}, {0, 1}, {1, 1}}}
};

struct CurrentPiece {
    struct Piece piece;
    struct Position position;
    char rotation;
    enum Colour colour;
};


char can_place_piece(
    char *screen,
    char x,
    char y,
    char rotation,
    struct Piece piece
) {
    for (int i = 0; i < piece.size; ++i) {
        char piece_x;
        char piece_y;
        struct Position* position = piece.definition + i;

        switch(rotation) {
            case 0: {
                piece_x = (*position).x;  
                piece_y = (*position).y;
                break;
            }
            case 1: {
                piece_x = -(*position).y;  
                piece_y = (*position).x;
                if (piece.moving_center) {
                    piece_x += 1;
                }
                break;
            }
            case 2: {
                piece_x = -(*position).x;  
                piece_y = -(*position).y;
                if (piece.moving_center) {
                    piece_x += 1;
                    piece_y += 1;
                }
                break;
            }
            case 3: {
                piece_x = (*position).y;  
                piece_y = -(*position).x;
                if (piece.moving_center) {
                    piece_y += 1;
                }
                break;
            }
        }

        char final_x = piece_x + x;
        char final_y = piece_y + y;
        if (final_x >= SCREEN_X) {
            return false;
        }
        if (final_x < 0) {
            return false;
        }
        if (final_y >= SCREEN_Y) {
            return false;
        }
        if (final_y < 0) {
            return true;
        }
        if (*(screen + final_x * SCREEN_Y + final_y)) {
            return false;
        }
    }
    return true;
}

char place_piece(
    char *board,
    char *screen,
    char x,
    char y,
    char rotation,
    struct Piece piece,
    enum Colour colour
) {
    if (can_place_piece(board, x, y, rotation, piece)) {
        copy_board((char *) screen, (char *) board);
        for (int i = 0; i < piece.size; ++i) {
            char piece_x;
            char piece_y;

            switch(rotation) {
                case 0: {
                    piece_x = (*(piece.definition + i)).x;  
                    piece_y = (*(piece.definition + i)).y;
                    break;
                }
                case 1: {
                    piece_x = -(*(piece.definition + i)).y;  
                    piece_y = (*(piece.definition + i)).x;
                    if (piece.moving_center) {
                        piece_x += 1;
                    }
                    break;
                }
                case 2: {
                    piece_x = -(*(piece.definition + i)).x;  
                    piece_y = -(*(piece.definition + i)).y;
                    if (piece.moving_center) {
                        piece_x += 1;
                        piece_y += 1;
                    }
                    break;
                }
                case 3: {
                    piece_x = (*(piece.definition + i)).y;  
                    piece_y = -(*(piece.definition + i)).x;
                    if (piece.moving_center) {
                        piece_y += 1;
                    }
                    break;
                }
            }

            char final_x = piece_x + x;
            char final_y = piece_y + y;
            if (final_y >= 0) {
                char* screen_colour = screen + final_x * SCREEN_Y + final_y;
                if (*screen_colour == 0) {
                    *screen_colour = colour;
                }
            }
        }
        renderer_render((char *) screen);
        return true;
    }
    return false;
}

void game_over(
    char *screen,
    char *board
) {
    for (int y = 0; y < SCREEN_Y; ++y) {
        for (int x = 0; x < SCREEN_X; ++x) {
            *(screen + x * SCREEN_Y + y) = COLOUR_RED;
        }

        renderer_render((char *) screen);
        renderer_delay(10);
    }

    for (int y = SCREEN_Y - 1; y >= 0; --y) {
        for (int x = 0; x < SCREEN_X; ++x) {
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
    int by = SCREEN_Y - 1;
    for (int y = SCREEN_Y - 1; y > 0; --y) {
        char row_is_full = true;
        for (int x = 0; x < SCREEN_X; ++x) {
            row_is_full = row_is_full && *(board + x * SCREEN_Y + y);
            *(board + x * SCREEN_Y + by) = *(board + x * SCREEN_Y + y);
        }
        if (!row_is_full) {
            --by;
        } else {
            renderer_play_sound(SOUND_DZIN);
            renderer_delay(SOUND_DZIN.length / 16);
        }
    }
    copy_board((char *) screen, (char *) board);
    renderer_render((char *) screen);
}

static int game_time = 0;
static char running = true;
enum Colour colours[7] = {COLOUR_RED, COLOUR_ORANGE, COLOUR_YELLOW, COLOUR_GREEN, COLOUR_BLUE, COLOUR_DEEP_BLUE, COLOUR_VIOLET}; 

int piece_down(
    char *board,
    char *screen,
    struct CurrentPiece *piece
) {
    if (place_piece((char *) board, (char *) screen, piece->position.x, piece->position.y + 1, piece->rotation, piece->piece, piece->colour)) {
        piece->position.y = piece->position.y + 1;
        return 1;
    } else {
        copy_board((char *) board, (char *) screen);
        check_board((char *) screen, (char *) board);
        piece->piece = pieces[rand() % pieces_number];
        piece->rotation = default_rotation;
        piece->position.y = 0;
        piece->position.x = START_X;
        piece->colour = colours[rand() % 7];
        if (!place_piece((char *) board, (char *) screen, piece->position.x, piece->position.y, piece->rotation, piece->piece, piece->colour)) {
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

    struct CurrentPiece piece = {pieces[rand() % pieces_number], {START_X, 0}, default_rotation, colours[rand() % 7]};
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
                    if (place_piece((char *) board, (char *) screen, piece.position.x - 1, piece.position.y, piece.rotation, piece.piece, piece.colour)) {
                        --piece.position.x;
                        renderer_play_sound(SOUND_CLICK);
                    }
                    break;
                }
                case EVENT_RIGHT: {
                    if (place_piece((char *) board, (char *) screen, piece.position.x + 1, piece.position.y, piece.rotation, piece.piece, piece.colour)) {
                        ++piece.position.x;
                        renderer_play_sound(SOUND_CLICK);
                    }
                    break;
                }
                case EVENT_DOWN: {
                    if (piece_down((char *) board, (char *) screen, &piece)) {
                        renderer_play_sound(SOUND_CLICK);
                    }
                    break;
                }
                case EVENT_UP: {
                    while (piece_down((char *) board, (char *) screen, &piece)) { renderer_delay(12); } 
                    renderer_play_sound(SOUND_SPOON2);
                    renderer_delay(200);
                    break;
                }
                case EVENT_SPACE: {
                    char next_rotation = piece.rotation + 1;
                    if (next_rotation > 3) next_rotation = 0;
                    if (place_piece((char *) board, (char *) screen, piece.position.x, piece.position.y, next_rotation, piece.piece, piece.colour)) {
                        piece.rotation = next_rotation;
                        renderer_play_sound(SOUND_SPOON);
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
