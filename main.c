#include <time.h>
#include <stdlib.h>

#define SCREEN_X 16
#define SCREEN_Y 24

#include "colours.h"
#include "sdl_renderer.h"

bool ONE_DEFINITION[1] = {true};
bool FOUR_DEFINITION[4] = {true, true, true, true};
bool G_BAR_DEFINITION[6] = {true, true, true, true, false, false};

struct Position {
    char x;
    char y;
};

struct Piece {
    char size_x;
    char size_y;
    bool *definition;
};

struct CurrentPiece {
    struct Piece piece;
    struct Position position;
    enum Colour colour;
};

const struct Piece g_bar = {2, 3, (bool *) G_BAR_DEFINITION};
const struct Piece square = {2, 2, (bool *) FOUR_DEFINITION};
const struct Piece bar = {1, 4, (bool *) FOUR_DEFINITION};
const struct Piece bar_2 = {4, 1, (bool *) FOUR_DEFINITION};

bool can_place_piece(
    char *screen,
    char x,
    char y,
    struct Piece piece
) {
    for (int i = 0; i < piece.size_x; ++i) {
        for (int j = 0; j < piece.size_y; ++j) {
            if (*(piece.definition + i * piece.size_y + j)) { 
                if (x + i >= SCREEN_X || x + i < 0 || y + j >= SCREEN_Y) {
                    return false;
                }
                if (*(screen + (x + i) * SCREEN_Y + (y + j))) {
                    return false;
                }
            }
	    }
    }
    return true;
}

bool place_piece(
    char *board,
    char *screen,
    char x,
    char y,
    struct Piece piece,
    enum Colour colour
) {
    if (can_place_piece(board, x, y, piece)) {
        memcpy(screen, board, SCREEN_X * SCREEN_Y * sizeof(char));
        for (int i = 0; i < piece.size_x; ++i) {
            for (int j = 0; j < piece.size_y; ++j) {
                char screen_colour = *(screen + (x + i) * SCREEN_Y + (y + j));
                if (screen_colour > 0) {
                    *(screen + (x + i) * SCREEN_Y + (y + j)) = screen_colour;
                } else if (*(piece.definition + i * piece.size_y + j)) {
                    *(screen + (x + i) * SCREEN_Y + (y + j)) = colour; 
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
struct Piece pieces[4] = {square, bar, bar_2, g_bar};
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


    struct CurrentPiece piece = {pieces[rand() % 4], {3, -1}, colours[rand() % 7]};
    while (running) {
        enum Event event = EVENT_EMPTY;
        do {
            event = renderer_get_event();
                switch(event) {
                    case EVENT_EXIT:
                    running = false;
                        break;
                    case EVENT_LEFT:
                        if (place_piece((char *) board, (char *) screen, piece.position.x - 1, piece.position.y, piece.piece, piece.colour)) {
                            --piece.position.x;
                        }
                        break;
                    case EVENT_RIGHT:
                        if (place_piece((char *) board, (char *) screen, piece.position.x + 1, piece.position.y, piece.piece, piece.colour)) {
                            ++piece.position.x;
                        }
                        break;
                    case EVENT_DOWN:
                        if (place_piece((char *) board, (char *) screen, piece.position.x, piece.position.y + 1, piece.piece, piece.colour)) {
                            ++piece.position.y;
                        }
                        break;
                    case EVENT_UP:
                        break;
                    case EVENT_SPACE:
                        break;
                }
        } while (event != EVENT_EMPTY);

        if (game_time == 300) {
            game_time = 0;
            memcpy(screen, board, SCREEN_X * SCREEN_Y * sizeof(char)); 
            if (place_piece((char *) board, (char *) screen, piece.position.x, piece.position.y + 1, piece.piece, piece.colour)) {
                piece.position.y++;
            } else {
                if (piece.position.y >= 0) {
                    place_piece((char *) board, (char *) screen, piece.position.x, piece.position.y, piece.piece, piece.colour);
                    check_board((char *) screen, (char *) board);
                } else {
                    game_over((char *) screen, (char *) board);
                }
                piece.piece = pieces[rand() % 4];
                piece.position.y = -1;
                piece.position.x = 3;
                piece.colour = colours[rand() % 7];
            }
        }

        game_time += 10;
        renderer_delay(10);

    }
    renderer_destroy();

    return 0;
}
