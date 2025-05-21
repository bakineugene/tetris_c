#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "events.h"

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
    if (SDL_Init(SDL_INIT_VIDEO)) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

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
    SDL_SetRenderDrawColor(renderer, 110, 177, 255, 255);
}

void renderer_render(bool *a) {
        SDL_RenderClear(renderer);


	for (int x = 0; x < SCREEN_X; ++x) {
	    for (int y = 0; y < SCREEN_Y; ++y) {
		if (*(a + x * SCREEN_Y + y) == true) {
                    SDL_Rect rect;
                    rect.x = 0 + x * 8 * 5;
                    rect.y = 0 + y * 8 * 5;
                    rect.w = 8 * 5;
                    rect.h = 8 * 5;

                    SDL_RenderDrawRect(renderer, &rect);
    	            SDL_SetRenderDrawColor(renderer, 230, 117, 117, 255);
                    SDL_RenderFillRect(renderer, &rect);
		}
	    }
	}
    	SDL_SetRenderDrawColor(renderer, 111, 105, 145, 255);
	for (int x = 0; x < 1 + SCREEN_X * 8 * 5; x += 8 * 5) {
            SDL_RenderDrawLine(renderer, x, 0, x, SCREEN_HEIGHT);
        }

	for (int y = 0; y < 1 + SCREEN_Y * 8 * 5; y += 8 * 5) {
            SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH, y);
        }
    	SDL_SetRenderDrawColor(renderer, 110, 177, 255, 255);

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
