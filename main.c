#include "renderer.h"
#include "game.h"

int main(int argc, char** argv) {
    Renderer renderer = new_renderer();
    renderer.init();

    game_start(renderer);

    renderer.destroy();

    return 0;
}
