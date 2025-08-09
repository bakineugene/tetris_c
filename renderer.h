#pragma once

#include "sounds.h"

typedef struct Renderer {
    enum Event (*get_event)(void);
    int (*init)(void);
    void (*render)(uint8_t *a);
    void (*delay)(int delay);
    void (*destroy)(void);
    void (*play_sound)(Sound sound);
} Renderer;

Renderer new_renderer(void);

