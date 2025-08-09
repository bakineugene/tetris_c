#if defined(__AVR__)
#include "avr/renderer.h"
#else
#include "sdl2/renderer.h"
#endif

#include "tetris/tetris.h"

int main(int argc, char** argv) {
    Renderer renderer = new_renderer();
    renderer.init();

    Tetris game = new_tetris(renderer);

    game_start(&game);

    renderer.destroy();

    return 0;
}
