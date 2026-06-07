# Tetris C — Project Reference

## Overview

Tetris written in pure C, targeting three platforms through a single codebase with dependency-injected hardware abstraction.

1. **Linux PC** — SDL2 renderer, keyboard + audio device (native `gcc`)
2. **Windows PC** — Same SDL2 renderer, cross-compiled with MinGW
3. **Atmega328P** — SPI-driven MAX7219 LED matrix, ADC joystick, PWM speaker (`avr-gcc`)

## Architecture

The entire project uses a **function-pointer vtable** pattern for hardware abstraction. The game logic contains zero platform-specific code.

```
main.c ──► Renderer new_renderer() ──► game_start(renderer)
                                              │
                               tetris/tetris.c (game loop)
                                              │
                      ┌───────────────────────┼───────────────────────┐
                      │                       │                       │
               renderer.h          ┌──────────┴──────────┐    ┌────────┴────────┐
               (Renderer struct)   │                     │    │                 │
                            sdl2/renderer.c        avr/renderer.c   avr/sound.h
                            (keyboard+SDL2)         (SPI+ADC)       (PWM+TIMER)
```

### The `Renderer` Interface

| Function Pointer | Purpose | Linux | AVR |
|---|---|---|---|
| `get_event()` | User input | Arrows + spacebar (SDL2 events) | ADC voltage divider → 6 directions |
| `init()` | Hardware init | SDL window + renderer + audio | SPI + Timer0/2 + ADC init |
| `render(uint8_t*)` | Draw screen buffer | SDL textures + colored rects | 6× MAX7219 via SPI (48×8 LEDs) |
| `delay(int)` | Frame timing | `SDL_Delay()` | Timer-based delay |
| `destroy()` | Cleanup | `SDL_Quit()` | SPI disable, timers off |
| `play_sound(Sound)` | One-shot SFX | SDL audio queue | Timer2 PWM + Timer0 ISR |

### The `Renderer` Struct (`renderer.h`)

```c
typedef struct Renderer {
    enum Event (*get_event)(void);
    int (*init)(void);
    void (*render)(uint8_t *a);
    void (*delay)(int delay);
    void (*destroy)(void);
    void (*play_sound)(Sound sound);
} Renderer;

Renderer new_renderer(void);
```

`new_renderer()` is implemented separately in `sdl2/renderer.c` and `avr/renderer.c` — create a platform-specific object with the correct function pointers.

## Directory Structure

```
.
├── main.c                    # Entry point, game loop boot
├── renderer.h                # Renderer vtable + new_renderer()
├── game.h                    # game_start(Renderer)
├── events.h                  # enum Event: LEFT, RIGHT, DOWN, UP, SPACE, EXIT
├── screen.h                  # SCREEN_X=16, SCREEN_Y=24 (logical resolution)
├── colours.h                 # enum Colour: RED..VIOLET, WALL
├── sounds.h                  # typedef struct Sound { length; start; }
├── generate_sound.py         # WAV → C header (build/sounds/*.h)
├── sounds/                   # Raw .wav audio assets
├── tetris/                   # Platform-neutral game logic
│   ├── tetris.h / .c         # Game state, loop, scoring, board management
│   ├── pieces.h / .c         # Tetromino definitions, rotation, collision
│   ├── position.h / .c       # Position struct, 4-orientation rotation
│   └── colours.h / .c        # 7 playable colour definitions
├── sdl2/
│   ├── renderer.c            # SDL2: window, keyboard, audio, texture rendering
│   ├── renderer.h            # new_renderer() declaration
│   └── sound.h               # SDL audio queue (8000Hz U8)
├── avr/
│   ├── renderer.c            # MAX7219 SPI, ADC joystick, Timer0 PWM
│   ├── renderer.h            # new_renderer() declaration
│   └── sound.h               # Timer2 speaker PWM driver
├── case/
│   └── max7219_small_mount.scad   # OpenSCAD 3D-print mount plate
├── build/                    # Generated (in .gitignore)
├── _deps/                    # SDL2 Windows headers (in .gitignore)
├── Makefile                  # Three build targets
└── AGENTS.md                 # This file
```

## Build Targets

### Linux / macOS (`build-sdl`)

```bash
make build-sdl    # Compile + link
make run          # Execute
```

- **Compiler:** `gcc` (native)
- **Sources:** `main.c + tetris/*.c + sdl2/renderer.c`
- **BMP:** Embedded via `ld -r -b binary` → `wall_bmp_native.o` (no filesystem needed)
- **Sound:** Python generates `build/sounds/*.h` with raw WAV sample arrays

### Windows (`build-windows`)

```bash
make build-windows   # Cross-compile via MinGW
make run-windows     # Run under Wine
```

- **Compiler:** `x86_64-w64-mingw32-gcc`
- **Sources:** Same `sdl2/renderer.c` (same source file, different toolchain)
- **SDL2 dependency:** Auto-downloaded to `_deps/SDL2-2.32.8/`
- **BMP:** MinGW linker binary blob → `wall_bmp_windows.o`

### AVR Microcontroller (`build-avr`, `upload-avr`)

```bash
make build-avr    # Compile for Atmega328P
make upload-avr   # Program via USBasp
```

- **Compiler:** `avr-gcc`
- **MCU:** `atmega328p` @ 16MHz, optimized `-Os`
- **Sources:** `tetris/*.c + avr/renderer.c` (swaps out SDL2)
- **Sound:** WAV headers use `PROGMEM` — samples stored in flash, copied to SRAM at playback
- **Output:** `build/main.hex` via `avr-objcopy`
- **Upload:** `avrdude -c usbasp -p m328p -U flash:w:"build/main.hex":a`

## Build Helpers

| Target | Description |
|---|---|
| `make clean` | Remove `build/` only (generated code) |
| `make clean-all` | Remove `build/` and `_deps/` (SDL2 download cache) |
| `make sounds` | Regenerate `build/sounds/*.h` from `sounds/*.wav` |
| `make build-avr` | Compile for AVR |
| `make upload-avr` | Build + upload via USBasp |

## Key Conventions

- **No platform specifics in `tetris/`** — game logic must not `#include` SDL or AVR headers
- **Sound data embedded at build time** — no runtime file reads for audio (Python generates C headers) or BMP (ld binary blob)
- **Logical screen is 16×24** — board display (10 columns + walls + score) mapped to this resolution
- **AVR sound generation** uses `#if defined(__AVR__)` to emit `PROGMEM` qualifiers
- Generated artifacts (`build/`, `_deps/`) are in `.gitignore`
- Raw assets in `sounds/` (`.wav`) and `case/` (`.scad`) are tracked in git

## Hardware

### AVR → Atmega328P

- **Display:** 6× MAX7219 LED matrix daisy-chained via SPI (48 columns × 8 rows)
- **Logical mapping:** 48 LEDs × 3 rows = 16 logical columns × 24 logical rows (OR: 16 logical columns mapped via block rendering)
- **Input:** Single-pin ADC joystick with voltage divider → detects 6 directions (up, down, left, right, hold, release)
- **Sound:** Speaker on PD3 via Timer2 frequency synthesis
- **Clock:** 16MHz external crystal

## Audio Pipeline

```
sounds/dzin.wav  ──► generate_sound.py ──► build/sounds/dzin.h
                                │
                        const uint8_t SOUND_DZIN_DATA[]
                        const Sound SOUND_DZIN = { length, DATA };
                                │
                        ADC at runtime │ AVR: PROGMEM → ISR 
                                ▼
                    play_sound(SOUND_DZIN)
```

## Texture Pipeline

```
sdl2/wall.bmp  ──► ld -r -b binary ──► wall_bmp.o ──► linked into .elf/.exe
                                   (_binary_sdl2_wall_bmp_start,
                                    _binary_sdl2_wall_bmp_end)
                                │
                        SDL_RWFromMem(start, size)
                        SDL_LoadBMP_RW(rw, 1)
                                ▼
                    SDL_Texture (no filesystem mount needed)
```

## Skills Reference

When working on this project, load these skills for target-specific guidance:

- **`skills/pc-version.md`** — SDL2 target (Linux + Windows). Read before modifying rendering, audio, input mapping, or cross-compilation.
- **`skills/microcontroller.md`** — AVR/Atmega328P target. Read before modifying hardware code, SPI, ADC, timers, or uploading firmware.