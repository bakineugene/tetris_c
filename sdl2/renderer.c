#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "renderer.h"
#include "../screen.h"
#include "../colours.h"
#include "../events.h"
#include "sound.h"

#define WINDOW_TITLE "Tetris on SDL2"

static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Event event;

struct SDLColour {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

static struct SDLColour renderer_colour_red = { 196, 54, 56 };
static struct SDLColour renderer_colour_orange = { 230, 100, 20 };
static struct SDLColour renderer_colour_yellow = { 224, 204, 26 };
static struct SDLColour renderer_colour_green = { 24, 226, 112 };
static struct SDLColour renderer_colour_blue = { 26, 189, 224 };
static struct SDLColour renderer_colour_deep_blue = { 25, 90, 225 };
static struct SDLColour renderer_colour_violet = { 169, 27, 222 };
static struct SDLColour renderer_colour_white = { 250, 250, 250 };

enum Event renderer_get_event() {
    if (SDL_PollEvent(&event)) {
	switch(event.type) {
	    case SDL_QUIT:
            return EVENT_EXIT;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                        return EVENT_LEFT;
                case SDLK_RIGHT:
                        return EVENT_RIGHT;
                case SDLK_DOWN:
                        return EVENT_DOWN;
                case SDLK_UP:
                        return EVENT_UP;
                case SDLK_SPACE:
                        return EVENT_SPACE;
            }
        }
    }
    return EVENT_EMPTY;
}

SDL_Texture* wall_texture;
void init_textures(void) {
    SDL_Surface* surface = SDL_LoadBMP("./sdl2/wall.bmp");
    if (!surface) {
        printf("Failed to load BMP: %s\n", SDL_GetError());
    }

    wall_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!wall_texture) {
        printf("Failed to create texture: %s\n", SDL_GetError());
    }
}

int side_size;

int renderer_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    renderer_init_sound();

    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);

    side_size = display_mode.h/ 30;

    window = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        side_size * SCREEN_X, side_size * SCREEN_Y, SDL_WINDOW_SHOWN);

    if (!window) {
        printf("Could not create a window: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    init_textures();
    if (!renderer) {
        printf("Could not create a renderer: %s\n", SDL_GetError());
        return -1;
    }

    SDL_SetRenderDrawColor(renderer, 30, 20, 40, 255);

    return 0;
}

void renderer_render(uint8_t *a) {
    SDL_RenderClear(renderer);

	for (int x = 0; x < SCREEN_X; ++x) {
	    for (int y = 0; y < SCREEN_Y; ++y) {
            struct SDLColour colour;
            bool has_colour = true;
            bool has_texture = false;
            switch(*(a + x * SCREEN_Y + y)) {
                case COLOUR_RED:
                    colour = renderer_colour_red;
                    break;
                case COLOUR_ORANGE:
                    colour = renderer_colour_orange;
                    break;
                case COLOUR_YELLOW:
                    colour = renderer_colour_yellow;
                    break;
                case COLOUR_GREEN:
                    colour = renderer_colour_green;
                    break;
                case COLOUR_BLUE:
                    colour = renderer_colour_blue;
                    break;
                case COLOUR_DEEP_BLUE:
                    colour = renderer_colour_deep_blue;
                    break;
                case COLOUR_VIOLET:
                    colour = renderer_colour_violet;
                    break;
                case COLOUR_WALL:
                    has_texture = true;
                    colour = renderer_colour_white;
                    break;
                default:
                    has_colour = false;
                    break;
            }
            if (has_colour) {
                SDL_Rect rect;
                rect.x = x * side_size;
                rect.y = y * side_size;
                rect.w = side_size;
                rect.h = side_size;

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 80);
                SDL_RenderDrawRect(renderer, &rect);

                int side_line_size = side_size / 20;

                if (!has_texture) {
                    rect.x += side_line_size;
                    rect.y += side_line_size;
                    rect.w -= side_line_size;
                    rect.h -= side_line_size;
                }
                SDL_SetRenderDrawColor(renderer, colour.red, colour.green, colour.blue, 80);
                SDL_RenderFillRect(renderer, &rect);
                if (has_texture) {
                    SDL_RenderCopy(renderer, wall_texture, NULL, &rect);
                }
            }
	    }
	}

    SDL_SetRenderDrawColor(renderer, 30, 20, 40, 255);

    SDL_RenderPresent(renderer);
}

void renderer_delay(int delay) {
    SDL_Delay(delay);
}

void renderer_destroy() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

Renderer new_renderer() {
    Renderer renderer;

    renderer.get_event = renderer_get_event;
    renderer.init = renderer_init;
    renderer.render = renderer_render;
    renderer.delay = renderer_delay;
    renderer.destroy = renderer_destroy;
    renderer.play_sound = renderer_play_sound;

    return renderer;
}

