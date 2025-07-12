#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "events.h"
#include "sdl_renderer_sound.h"

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"

/* macros */
#define SCREEN_WIDTH 40 * SCREEN_X
#define SCREEN_HEIGHT 40 * SCREEN_Y
#define WINDOW_TITLE "Tetris on SDL2"

/* variables */
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Event event;

struct SDLColour {
    char red;
    char green;
    char blue;
};

static struct SDLColour renderer_colour_red = { 196, 54, 56 };
static struct SDLColour renderer_colour_orange = { 230, 100, 20 };
static struct SDLColour renderer_colour_yellow = { 224, 204, 26 };
static struct SDLColour renderer_colour_green = { 24, 226, 112 };
static struct SDLColour renderer_colour_blue = { 26, 189, 224 };
static struct SDLColour renderer_colour_deep_blue = { 25, 90, 225 };
static struct SDLColour renderer_colour_violet = { 169, 27, 222 };

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

int renderer_init(void) {
    /* Initializing SDL2 */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    renderer_init_sound();

    /* Creating a SDL window */
    window = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if (!window) {
        printf("Could not create a window: %s\n", SDL_GetError());
        return -1;
    }

    /* Creating a renderer */
    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer) {
        printf("Could not create a renderer: %s\n", SDL_GetError());
        return -1;
    }

    /* Background color               r    g    b    a */
    // SDL_SetRenderDrawColor(renderer, 110, 177, 255, 255);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_SetRenderDrawColor(renderer, 30, 20, 40, 255);

    return 0;
}

void renderer_render(char *a) {
    SDL_RenderClear(renderer);

	for (int x = 0; x < SCREEN_X; ++x) {
	    for (int y = 0; y < SCREEN_Y; ++y) {
            struct SDLColour colour;
            bool has_colour = true;
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
                default:
                    has_colour = false;
                    break;
            }
            if (has_colour) {
                SDL_Rect rect;
                rect.x = 0 + x * 8 * 5;
                rect.y = 0 + y * 8 * 5;
                rect.w = 8 * 5;
                rect.h = 8 * 5;

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 80);
                SDL_RenderDrawRect(renderer, &rect);
                rect.x += 2;
                rect.y += 2;
                rect.w -= 2;
                rect.h -= 2;
                SDL_SetRenderDrawColor(renderer, colour.red, colour.green, colour.blue, 80);
                SDL_RenderFillRect(renderer, &rect);
            }
	    }
	}
    /**
    SDL_SetRenderDrawColor(renderer, 111, 105, 145, 255);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	for (int x = 0; x < 1 + SCREEN_X * 8 * 5; x += 8 * 5) {
        SDL_RenderDrawLine(renderer, x, 0, x, SCREEN_HEIGHT);
    }

	for (int y = 0; y < 1 + SCREEN_Y * 8 * 5; y += 8 * 5) {
        SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH, y);
    }
    */
    SDL_SetRenderDrawColor(renderer, 110, 177, 255, 255);
    SDL_SetRenderDrawColor(renderer, 30, 20, 40, 255);

    /* Showing what was drawn */
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
