#pragma once

typedef struct Renderer {
    enum Event (*get_event)(void);
    int (*init)(void);
    void (*render)(uint8_t *a);
    void (*delay)(int delay);
    void (*destroy)(void);
} Renderer;

