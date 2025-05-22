#include <time.h>
#include <stdlib.h>

#define SCREEN_X 16
#define SCREEN_Y 24

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
};

const struct Piece g_bar = {2, 3, (bool *) G_BAR_DEFINITION};
const struct Piece square = {2, 2, (bool *) FOUR_DEFINITION};
const struct Piece bar = {1, 4, (bool *) FOUR_DEFINITION};
const struct Piece bar_2 = {4, 1, (bool *) FOUR_DEFINITION};

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
struct Piece pieces[4] = {square, bar, bar_2, g_bar};

int main(int argc, char** argv) {
    srand(time(NULL));
    
    renderer_init();

    bool board[SCREEN_X][SCREEN_Y];
    bool screen[SCREEN_X][SCREEN_Y];

    for (int x = 0; x < SCREEN_X; ++x) {
        for (int y = 0; y < SCREEN_Y; ++y) {
	        board[x][y] = false;
	    }
    }


    struct CurrentPiece piece = {pieces[rand() % 4], {3, -1}};
    while (running) {
        enum Event event = EVENT_EMPTY;
        do {
            event = renderer_get_event();
                switch(event) {
                    case EVENT_EXIT:
                    running = false;
                        break;
                    case EVENT_LEFT:
                        if (can_place_piece((bool *) board, piece.position.x - 1, piece.position.y, piece.piece)) {
                            --piece.position.x;
                            place_piece((bool *) board, (bool *) screen, piece.position.x, piece.position.y, piece.piece);
                        }
                        break;
                    case EVENT_RIGHT:
                        if (can_place_piece((bool *) board, piece.position.x + 1, piece.position.y, piece.piece)) {
                            ++piece.position.x;
                            place_piece((bool *) board, (bool *) screen, piece.position.x, piece.position.y, piece.piece);
                        }
                        break;
                    case EVENT_DOWN:
                        if (can_place_piece((bool *) board, piece.position.x, piece.position.y + 1, piece.piece)) {
                            ++piece.position.y;
                            place_piece((bool *) board, (bool *) screen, piece.position.x, piece.position.y, piece.piece);
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
            if (can_place_piece((bool *) screen, piece.position.x, piece.position.y + 1, piece.piece)) {
                place_piece((bool *) board, (bool *) screen, piece.position.x, piece.position.y + 1, piece.piece);
                piece.position.y++;
            } else {
                if (piece.position.y >= 0) {
                    place_piece((bool *) board, (bool *) screen, piece.position.x, piece.position.y, piece.piece);
                    check_board((bool *) screen, (bool *) board);
                } else {
                    game_over((bool *) screen, (bool *) board);
                }
                piece.piece = pieces[rand() % 4];
                piece.position.y = -1;
                piece.position.x = 3;
            }
        }

        game_time += 10;
        renderer_delay(10);

    }
    renderer_destroy();

    return 0;
}
