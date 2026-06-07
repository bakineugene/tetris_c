# SDL2 Target Skill

## Scope

This target covers the PC build: same `sdl2/renderer.c` compiled on Linux (native `gcc`) or Windows (MinGW cross-compile). Use this skill for:

- Modifying renderer behavior (input, drawing, sounds)
- Adding new visual elements (textures, UI pixels)
- Fixing cross-platform issues
- Tweaking audio playback

## Key Files

| File | Read only | Modify | Purpose |
|---|---|---|---|
| `sdl2/renderer.c` | ✓ | ✓ | Input mapping, rendering, `new_renderer()` |
| `sdl2/sound.h` | ✓ | ✓ | Audio device init, play_sound() |
| `sdl2/renderer.h` | ✓ | ✓ | `new_renderer()` declaration |
| `tetris/tetris.c` | ✓ | ✓ | Game loop (buffer → render) |
| `colours.h` | ✓ | ✓ | Define new pattern colours |
| `events.h` | ✓ | ✓ | Enum `Event` variants |
| `screen.h` | ✓ | ✓ | `SCREEN_X=16`, `SCREEN_Y=24` |

## Do Not Modify

The following files in `tetris/` are platform-neutral. **Never** add `#include <SDL2/SDL.h>` or hardware calls in `tetris/*.c`. That's the whole point of the `Renderer` vtable.

- `tetris/tetris.c`, `tetris/tetris.h`
- `tetris/pieces.c`, `tetris/pieces.h`
- `tetris/position.c`, `tetris/position.h`
- `tetris/colours.c`, `tetris/colours.h`
- `tetris/board.h`
- `renderer.h`, `game.h`, `main.c`

## Building

```bash
make build-sdl   # Native Linux
make build-windows  # Cross-compile for Windows (Wine)
make clean         # Shallow: remove build/
make clean-all     # Deep: remove build/ + _deps/
```

## Implementing Input

Add new input mappings in `renderer.c → renderer_get_event()`:

```c
switch (event.key.keysym.sym) {
    case SDLK_LEFT:        return EVENT_LEFT;
    case SDLK_RIGHT:       return EVENT_RIGHT;
    case SDLK_DOWN:        return EVENT_DOWN;
    case SDLK_UP:          return EVENT_UP;       // Hard drop (current I-piece center)
    case SDLK_SPACE:       return EVENT_SPACE;     // Echo piece
    case SDLK_ESCAPE:      return EVENT_EXIT;
}
```

For new inputs, add entries to `enum Event` in `events.h` before wiring them into tetris logic.

## Implementing Sound

Sound data is generated at build time by `generate_sound.py`. The pipeline:

```
sounds/*.wav  →  python3 generate_sound.py <name>  →  build/sounds/<name>.h
                              │
                      const uint8_t SOUND_NAME_DATA[]
                      const Sound SOUND_NAME = { length, DATA };
```

### Adding new sounds

1. Place a `.wav` file in `sounds/` (8-bit mono, ≤16kHz)
2. Add a recipe in `Makefile`:
   ```make
   build/sounds/newname.h: sounds/newname.wav
       mkdir -p build/sounds
       python3 generate_sound.py newname
   ```
3. Add to the `sounds` target line: `build/sounds/newname.h`
4. Include in the game with `#include "../../build/sounds/newname.h"`

### Sound API

`sdl2/sound.h` provides two functions:

```c
void renderer_play_sound(Sound sound);     // Called from game logic
int renderer_init_sound();                 // Called from renderer_init()
```

Audio format: 8000Hz, unsigned 8-bit mono, 512-byte queue buffer.

### Adding sound playback in the game

In `sdl2/renderer.c`, add a case to the `switch` that dispatches to `play_sound`:

```c
case EVENT_PLACE:
    renderer_play_sound(SOUND_CLICK);
    break;
```

## Implementing Rendering

`renderer.c → renderer_render()` receives the 16×24 screen buffer (`uint8_t* a`). Each cell value determines colour via `switch`:

```c
switch(*(a + x * SCREEN_Y + y)) {
    case COLOUR_RED:    /* falls with red block */
    case COLOUR_WALL:   /* draws wall texture */
    default:            /* empty cell */
}
```

### New patterns

1. Add colour enum value to `colours.h`
2. Add `case COLOUR_*` in `renderer_render()` with appropriate fill
3. Add the pattern colour to `tetris/colours.c` array

### Wall texture (BMP)

The wall tile uses a BMP embedded at link time (`ld -r -b binary`). It's loaded once into an `SDL_Texture`:

```c
SDL_Texture* wall_texture;

void init_textures(void) {
    extern const char _binary_sdl2_wall_bmp_start[];
    extern const char _binary_sdl2_wall_bmp_end[];
    size_t size = _binary_sdl2_wall_bmp_end - _binary_sdl2_wall_bmp_start;
    SDL_RWops* rw = SDL_RWFromMem((void*)_binary_sdl2_wall_bmp_start, (int)size);
    wall_texture = SDL_CreateTextureFromSurface(renderer, SDL_LoadBMP_RW(rw, 0));
}
```

### Updating wall.bmp

1. Edit `sdl2/wall.bmp` with any image editor (57K, 119×121 RGB)
2. Don't commit it — it's embedded at build time and in `.gitignore`
3. To debug changes: `make build-sdl && ./build/sdl_tetris`

## Common Pitfalls

### Relative paths don't work on Windows

Always use `SDL_GetBasePath()` or embedded binary blobs. The old `"./sdl2/wall.bmp"` approach fails because the working directory depends on how/where the `.exe` was launched.

### Sound format validation

The generator enforces: mono, 8-bit PCM. If your WAV is stereo or 16-bit, Python will exit with an error. Use `ffmpeg` to convert:

```bash
ffmpeg -i input.wav -f u8 -ac 1 -ar 8000 sounds/out.wav
```

### Block drawing geometry

Blocks are drawn as inset rectangles (indent ~5% from edge). When adding borders or gradients, account for `rect.x += side_line_size` after `side_size / 20`.

### Colour palette mapping

Each tetromino shape gets a random colour from the 7-element array in `tetris/colours.c`. Colours are assigned by index (`red=0`, `green=1`, etc.) to deterministic piece colouring.

### Performance

- `renderer_render()` iterates all 384 cells every frame — keep switches tight
- BMP blob is ~57KB in `.rodata` — ~4× sound data total (not memory intensive on PC)
- Tight back-n-forth between `tetris.c` and `tetris/colours.c` pattern to determine pick-vs-pull