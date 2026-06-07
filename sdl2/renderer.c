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

int side_size;

/* ── event polling ── */

enum Event renderer_get_event(void) {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            return EVENT_EXIT;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_LEFT:             return EVENT_LEFT;
            case SDLK_RIGHT:            return EVENT_RIGHT;
            case SDLK_DOWN:             return EVENT_DOWN;
            case SDLK_UP:               return EVENT_UP;
            case SDLK_SPACE:            return EVENT_SPACE;
            case SDLK_F11:
            case SDLK_ESCAPE:
            {
                if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP)
                    SDL_SetWindowFullscreen(window, 0);
                else if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)
                    SDL_SetWindowFullscreen(window, 0);
                else
                    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                return EVENT_FULLSCREEN;
            }
            }
            break;
        }
    }
    return EVENT_EMPTY;
}

/* ── texture init ── */

extern const char _binary_sdl2_wall_bmp_start[];
extern const char _binary_sdl2_wall_bmp_end[];

SDL_Texture* wall_texture;

void init_textures(void) {
    size_t bmp_size = (size_t)(_binary_sdl2_wall_bmp_end - _binary_sdl2_wall_bmp_start);
    SDL_RWops* rw = SDL_RWFromMem((void*)_binary_sdl2_wall_bmp_start, (int)bmp_size);
    SDL_Surface* surface = SDL_LoadBMP_RW(rw, 1);
    if (!surface)
        fprintf(stderr, "Failed to load BMP: %s\n", SDL_GetError());
    wall_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!wall_texture)
        fprintf(stderr, "Failed to create wall texture: %s\n", SDL_GetError());
}

/* ── init ── */

int renderer_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }
    renderer_init_sound();

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    side_size = dm.h / 30;

    SDL_Window* w = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        side_size * SCREEN_X, side_size * SCREEN_Y,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE);
    if (!w) {
        printf("Could not create a window: %s\n", SDL_GetError());
        return -1;
    }

    window = w;

    SDL_SetWindowResizable(window, SDL_TRUE);

    /* minimum size — game area (board + borders) */
    SDL_SetWindowMinimumSize(window,
        SCREEN_X * side_size + 2,
        SCREEN_Y * side_size + 2);

    SDL_Renderer* r = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!r) {
        printf("Could not create a renderer: %s\n", SDL_GetError());
        return -1;
    }

    renderer = r;

    init_textures();

    /* Set cell size from initial display mode */
    SDL_GetCurrentDisplayMode(0, &dm);
    side_size = dm.h / 30;

    return 0;
}

/* ── render ── */

static const uint8_t clr_red[3]    = {196, 54,  56};
static const uint8_t clr_orange[3] = {230,100, 20};
static const uint8_t clr_yellow[3] = {224,204, 26};
static const uint8_t clr_green[3]  = { 24,226,112};
static const uint8_t clr_blue[3]   = { 26,189,224};
static const uint8_t clr_deepblue[3]={25, 90,225};
static const uint8_t clr_violet[3] = {169, 27,222};
static const uint8_t clr_white[3]  = {250,250,250};

static void set_clr(SDL_Renderer* r, const uint8_t c[3]) {
    SDL_SetRenderDrawColor(r, c[0], c[1], c[2], 255);
}

void renderer_render(uint8_t *a) {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    /* background — 30, 20, 40 */
    SDL_SetRenderDrawColor(renderer, 30, 20, 40, 255);
    SDL_RenderClear(renderer);

    /* 1px white border around game rect, centred */
    int gw = SCREEN_X * side_size + 2;
    int gh = SCREEN_Y * side_size + 2;
    int ox = (w - gw) / 2;
    int oy = (h - gh) / 2;
    SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    SDL_RenderDrawRect(renderer, &(SDL_Rect){ ox, oy, gw, gh });

    int sx = ox + 1;          /* inner cell origin */
    int sy = oy + 1;

    /* draw grid cells */
    for (int bx = 0; bx < SCREEN_X; ++bx) {
        for (int by = 0; by < SCREEN_Y; ++by) {
            uint8_t v = *(a + bx * SCREEN_Y + by);
            if (v == 0) continue;                  /* empty — skip */

            SDL_Rect rect;
            rect.x = sx + bx * side_size;
            rect.y = sy + by * side_size;
            rect.w = side_size;
            rect.h = side_size;

            bool is_wall = (v == COLOUR_WALL);

            /* cell outline */
            SDL_SetRenderDrawColor(renderer, 30, 20, 40, 255);
            SDL_RenderDrawRect(renderer, &rect);

            /* inset fill */
            int in = side_size / 20;
            if (!is_wall) {
                rect.x += in; rect.y += in;
                rect.w -= in; rect.h -= in;
            }
            switch (v) {
            case COLOUR_RED:       set_clr(renderer, clr_red);       break;
            case COLOUR_ORANGE:    set_clr(renderer, clr_orange);    break;
            case COLOUR_YELLOW:    set_clr(renderer, clr_yellow);    break;
            case COLOUR_GREEN:     set_clr(renderer, clr_green);     break;
            case COLOUR_BLUE:      set_clr(renderer, clr_blue);      break;
            case COLOUR_DEEP_BLUE: set_clr(renderer, clr_deepblue);  break;
            case COLOUR_VIOLET:    set_clr(renderer, clr_violet);    break;
            case COLOUR_WALL:      set_clr(renderer, clr_white);     break;
            default:               continue;
            }
            SDL_RenderFillRect(renderer, &rect);

            if (is_wall)
                SDL_RenderCopy(renderer, wall_texture, NULL, &rect);
        }
    }

    SDL_SetRenderDrawColor(renderer, 30, 20, 40, 255);
    SDL_RenderPresent(renderer);
}

/* ── lifecycle ── */

void renderer_delay(int delay) {
    SDL_Delay(delay);
}

void renderer_destroy(void) {
    /* restore windowed before teardown */
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP)
        SDL_SetWindowFullscreen(window, 0);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

Renderer new_renderer(void) {
    Renderer r;
    r.get_event  = renderer_get_event;
    r.init       = renderer_init;
    r.render     = renderer_render;
    r.delay      = renderer_delay;
    r.destroy    = renderer_destroy;
    r.play_sound = renderer_play_sound;
    return r;
}