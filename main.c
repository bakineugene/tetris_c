#include <time.h>
#include <stdlib.h>

#define SCREEN_X 16
#define SCREEN_Y 24

#include "sdl_renderer.h"

bool ONE_DEFINITION[1] = {true};
bool FOUR_DEFINITION[4] = {true, true, true, true};
bool G_BAR_DEFINITION[6] = {true, true, true, true, false, false};

struct Piece {
    char size_x;
    char size_y;
    bool *definition;
};

struct Piece g_bar = {2, 3, (bool *) G_BAR_DEFINITION};
struct Piece square = {2, 2, (bool *) FOUR_DEFINITION};
struct Piece bar = {1, 4, (bool *) FOUR_DEFINITION};
struct Piece bar_2 = {4, 1, (bool *) FOUR_DEFINITION};

bool can_place_piece(
    bool *screen,
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

void place_piece(
    bool *board,
    bool *screen,
    char x,
    char y,
    struct Piece piece
) {
    memcpy(screen, board, SCREEN_X * SCREEN_Y * sizeof(bool));
    for (int i = 0; i < piece.size_x; ++i) {
        for (int j = 0; j < piece.size_y; ++j) {
            *(screen + (x + i) * SCREEN_Y + (y + j)) = *(screen + (x + i) * SCREEN_Y + (y + j)) || *(piece.definition + i * piece.size_y + j);
        }
    }
    renderer_render((bool *) screen);
}

void game_over(
    bool *screen,
    bool *board
) {
    for (int y = 0; y < SCREEN_Y; ++y) {
        for (int x = 0; x < SCREEN_X; ++x) {
            *(screen + x * SCREEN_Y + y) = true;
        }

        renderer_render((bool *) screen);
        renderer_delay(100);
    }

    for (int y = SCREEN_Y - 1; y >= 0; --y) {
        for (int x = 0; x < SCREEN_X; ++x) {
            *(screen + x * SCREEN_Y + y) = false;
            *(board + x * SCREEN_Y + y) = false;
        }

        renderer_render((bool *) screen);
        renderer_delay(100);
    }
}

void check_board(
    bool *screen,
    bool *board
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
    memcpy(screen, board, SCREEN_X * SCREEN_Y * sizeof(bool));
    renderer_render((bool *) screen);
}

static int game_time = 0;
static bool running = true;
static char piece_position_x = 3;

int main(int argc, char** argv) {
    renderer_init();

    bool board[SCREEN_X][SCREEN_Y];
    bool screen[SCREEN_X][SCREEN_Y];

    for (int x = 0; x < SCREEN_X; ++x) {
        for (int y = 0; y < SCREEN_Y; ++y) {
	    board[x][y] = false;
	}
    }

    srand(time(NULL));

    struct Piece pieces[4] = {square, bar, bar_2, g_bar};
    char piece_number = rand() % 4;
    struct Piece current_piece = pieces[piece_number];
    char current_piece_y = -1;
    while (running) {
        enum Event event = EVENT_EMPTY;
        do {
            event = renderer_get_event();
                switch(event) {
                    case EVENT_EXIT:
                    running = false;
                        break;
                    case EVENT_LEFT:
                        if (can_place_piece((bool *) board, piece_position_x - 1, current_piece_y, current_piece)) {
                            --piece_position_x;
                            place_piece((bool *) board, (bool *) screen, piece_position_x, current_piece_y, current_piece);
                        }
                        break;
                    case EVENT_RIGHT:
                        if (can_place_piece((bool *) board, piece_position_x + 1, current_piece_y, current_piece)) {
                            ++piece_position_x;
                            place_piece((bool *) board, (bool *) screen, piece_position_x, current_piece_y, current_piece);
                        }
                        break;
                    case EVENT_DOWN:
                        if (can_place_piece((bool *) board, piece_position_x, current_piece_y + 1, current_piece)) {
                            ++current_piece_y;
                            place_piece((bool *) board, (bool *) screen, piece_position_x, current_piece_y, current_piece);
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
            memcpy(screen, board, SCREEN_X * SCREEN_Y * sizeof(bool)); 
            if (can_place_piece((bool *) screen, piece_position_x, current_piece_y + 1, current_piece)) {
                place_piece((bool *) board, (bool *) screen, piece_position_x, current_piece_y + 1, current_piece);
                current_piece_y++;
            } else {
                if (current_piece_y >= 0) {
                    place_piece((bool *) board, (bool *) screen, piece_position_x, current_piece_y, current_piece);
                    check_board((bool *) screen, (bool *) board);
                } else {
                    game_over((bool *) screen, (bool *) board);
                }
                piece_number = rand() % 4;
                current_piece = pieces[piece_number];
                current_piece_y = -1;
                piece_position_x = 3;
            }
        }

        game_time += 10;
        renderer_delay(10);

    }
    renderer_destroy();

    return 0;
}
