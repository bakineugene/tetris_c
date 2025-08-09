#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define SCREEN_X 16
#define SCREEN_Y 24

#include "colours.h"

#if defined(__AVR__)
#include <avr/pgmspace.h>
#include "avr/renderer.h"
#else
#include <stdlib.h>
#include "sdl2/renderer.h"
#endif

#include "tetris/tetris.h"

int main(int argc, char** argv) {
    srand(time(NULL));
    renderer_init();

    Tetris game = game_new();

    game_start(&game);

    renderer_destroy();

    return 0;
}
