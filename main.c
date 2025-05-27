#include <time.h>
#include <stdlib.h>

#define SCREEN_X 16
#define SCREEN_Y 24

#include "colours.h"
#include "sdl_renderer.h"

struct Position {
    char x;
    char y;
};

struct Piece {
    char size;
    bool moving_center;
    struct Position *definition;
};

static char pieces_number = 7;
static const char default_rotation = 0;

struct Position i_definition[4] = {{-1, 0}, {0, 0}, {1, 0}, {2, 0}};
static const struct Piece i_bar = {4, true, i_definition};

struct Position j_definition[4] = {{0, -1}, {0, 0}, {0, 1}, {-1, 1}};
static const struct Piece j_bar = {4, false, j_definition};

struct Position l_definition[4] = {{0, -1}, {0, 0}, {0, 1}, {1, 1}};
static const struct Piece l_bar = {4, false, l_definition};

struct Position o_definition[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
static const struct Piece o_bar = {4, true, o_definition};

struct Position s_definition[4] = {{0, 0}, {1, 0}, {0, 1}, {-1, 1}};
static const struct Piece s_bar = {4, false, s_definition};

struct Position t_definition[4] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}};
static const struct Piece t_bar = {4, false, t_definition};

struct Position z_definition[4] = {{0, 0}, {-1, 0}, {0, 1}, {1, 1}};
static const struct Piece z_bar = {4, false, z_definition};

struct Piece pieces[7] = {i_bar, j_bar, l_bar, o_bar, s_bar, t_bar, z_bar};

struct CurrentPiece {
    struct Piece piece;
    struct Position position;
    char rotation;
    enum Colour colour;
};

bool can_place_piece(
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
        if (final_x >= SCREEN_X || final_x < 0 || final_y >= SCREEN_Y) {
            return false;
        }
        if (*(screen + final_x * SCREEN_Y + final_y)) {
            return false;
        }
    }
    return true;
}

bool place_piece(
    char *board,
    char *screen,
    char x,
    char y,
    char rotation,
    struct Piece piece,
    enum Colour colour
) {
    if (can_place_piece(board, x, y, rotation, piece)) {
        memcpy(screen, board, SCREEN_X * SCREEN_Y * sizeof(char));
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
            char* screen_colour = screen + final_x * SCREEN_Y + final_y;
            if (*screen_colour == 0) {
                *screen_colour = colour;
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
        renderer_delay(100);
    }

    for (int y = SCREEN_Y - 1; y >= 0; --y) {
        for (int x = 0; x < SCREEN_X; ++x) {
            *(screen + x * SCREEN_Y + y) = 0;
            *(board + x * SCREEN_Y + y) = 0;
        }

        renderer_render((char *) screen);
        renderer_delay(100);
    }
}

void check_board(
    char *screen,
    char *board
) {
    int by = SCREEN_Y - 1;
    for (int y = SCREEN_Y - 1; y > 0; --y) {
        bool row_is_full = true;
        for (int x = 0; x < SCREEN_X; ++x) {
            row_is_full = row_is_full && *(screen + x * SCREEN_Y + y);
            *(board + x * SCREEN_Y + by) = *(screen + x * SCREEN_Y + y);
        }
        if (!row_is_full) {
            --by;
        }
    }
    memcpy(screen, board, SCREEN_X * SCREEN_Y * sizeof(char));
    renderer_render((char *) screen);
}

static int game_time = 0;
static bool running = true;
enum Colour colours[7] = {COLOUR_RED, COLOUR_ORANGE, COLOUR_YELLOW, COLOUR_GREEN, COLOUR_BLUE, COLOUR_DEEP_BLUE, COLOUR_VIOLET};
 

int main(int argc, char** argv) {
    srand(time(NULL));
    
    renderer_init();

    char board[SCREEN_X][SCREEN_Y];
    char screen[SCREEN_X][SCREEN_Y];

    for (int x = 0; x < SCREEN_X; ++x) {
        for (int y = 0; y < SCREEN_Y; ++y) {
	        board[x][y] = false;
	    }
    }

    struct CurrentPiece piece = {pieces[rand() % pieces_number], {5, 0}, default_rotation, colours[rand() % 7]};
    while (running) {
        enum Event event = EVENT_EMPTY;
        do {
            event = renderer_get_event();
                switch(event) {
                    case EVENT_EXIT:
                    running = false;
                        break;
                    case EVENT_LEFT:
                        if (place_piece((char *) board, (char *) screen, piece.position.x - 1, piece.position.y, piece.rotation, piece.piece, piece.colour)) {
                            --piece.position.x;
                        }
                        break;
                    case EVENT_RIGHT:
                        if (place_piece((char *) board, (char *) screen, piece.position.x + 1, piece.position.y, piece.rotation, piece.piece, piece.colour)) {
                            ++piece.position.x;
                        }
                        break;
                    case EVENT_DOWN:
                        if (place_piece((char *) board, (char *) screen, piece.position.x, piece.position.y + 1, piece.rotation, piece.piece, piece.colour)) {
                            ++piece.position.y;
                        }
                        break;
                    case EVENT_UP:
                        break;
                    case EVENT_SPACE:
                        char next_rotation = piece.rotation + 1;
                        if (next_rotation > 3) next_rotation = 0;
                        if (place_piece((char *) board, (char *) screen, piece.position.x, piece.position.y, next_rotation, piece.piece, piece.colour)) {
                            piece.rotation = next_rotation;
                        }
                        break;
                }
        } while (event != EVENT_EMPTY);

        if (game_time == 300) {
            game_time = 0;
            memcpy(screen, board, SCREEN_X * SCREEN_Y * sizeof(char)); 
            if (place_piece((char *) board, (char *) screen, piece.position.x, piece.position.y + 1, piece.rotation, piece.piece, piece.colour)) {
                piece.position.y++;
            } else {
                if (piece.position.y >= 0) {
                    place_piece((char *) board, (char *) screen, piece.position.x, piece.position.y, piece.rotation, piece.piece, piece.colour);
                    check_board((char *) screen, (char *) board);
                } else {
                    game_over((char *) screen, (char *) board);
                }
                piece.piece = pieces[rand() % pieces_number];
                piece.rotation = default_rotation;
                piece.position.y = 0;
                piece.position.x = 5;
                piece.colour = colours[rand() % 7];
            }
        }

        game_time += 10;
        renderer_delay(10);

    }
    renderer_destroy();

    return 0;
}
