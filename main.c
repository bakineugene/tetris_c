#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#include "screen.h"
#include "colours.h"

#if defined(__AVR__)
#include "avr/renderer.h"
#else
#include "sdl2/renderer.h"
#endif

#include "tetris/tetris.h"

int main(int argc, char** argv) {
    renderer_init();

    Tetris game = game_new();

    game_start(&game);

    renderer_destroy();

    return 0;
}
