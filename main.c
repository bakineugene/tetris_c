#include "renderer.h"
#include "tetris/tetris.h"

int main(int argc, char** argv) {
    Renderer renderer = new_renderer();
    renderer.init();

    Tetris game = new_tetris(renderer);

    game_start(&game);

    renderer.destroy();

    return 0;
}
