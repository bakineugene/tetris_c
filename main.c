#include "sdl_renderer.h"

static bool running;

void draw_peace(
    bool *screen,
    char x,
    char y,
    char size_x,
    char size_y,
    bool *peace
) {
    for (int i = 0; i < size_x; ++i) {
        for (int j = 0; j < size_y; ++j) {
	    *(screen + (x + i) * 16 + (y + j)) = *(peace + i * size_y + j);
	}
    }
}

bool dot[1] = {true};
bool square[2][2] = {true, true, true, true};
bool bar[1][4] = {true, true, true, true};
bool g_bar[2][3] = {true, true, true, true, false, false};

int main(int argc, char** argv) {
    renderer_init();
    running = true;

    bool board[8][16] = {0};
    bool screen[8][16];

    char i = 0;
    while (running) {
        running = renderer_running();

	memcpy(screen, board, 8 * 16 * sizeof(bool)); 

	draw_peace((bool *) screen, 3, i, 2, 3, (bool *) g_bar);
	if (++i == 16) i = 0;

	renderer_delay(300);

	renderer_render((bool *) screen);
    }
    renderer_destroy();

    return 0;
}
