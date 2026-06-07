# AVR Microcontroller Target

## Scope

This target covers the Atmega328P build for the physical Tetris hardware. Use this skill for:

- Modifying `avr/renderer.c` (SPI LED matrix, ADC input, timers)
- Adjusting audio output (Timer2 PWM speaker)
- Power management, dimming, or physical display tweaks
- Programming / flashing via USBasp

## Key Files

| File | Read only | Modify | Purpose |
|---|---|---|---|
| `avr/renderer.c` | ✓ | ✓ | MAX7219 SPI, ADC, Timer0/Timer2 |
| `avr/renderer.h` | ✓ | ✓ | `new_renderer()` declaration |
| `avr/sound.h` | ✓ | ✓ | Timer2 speaker PWM driver |
| `generate_sound.py` | ✓ | ✓ | `sound.h` generator |
| `tetris/tetris.c` | ✓ | ✓ | Game loop (buffer → render) |
| `colours.h` | ✓ | ✓ | LED colour enum (hardware is monochrome) |
| `screen.h` | ✓ | ✓ | `SCREEN_X=16` logical, maps to SPI rows |

**AVR `__AVR__` guards in `generate_sound.py` emit PROGMEM arrays** — keep these in sync.

## Do Not Modify

The following files in `tetris/` are platform-neutral. **Never** add `#include <avr/io.h>` or hardware calls in `tetris/*.c`. That's the whole point of the `Renderer` vtable.

- `tetris/tetris.c`, `tetris/tetris.h`
- `tetris/pieces.c`, `tetris/pieces.h`
- `tetris/position.c`, `tetris/position.h`
- `tetris/colours.c`, `tetris/colours.h`
- `tetris/board.h`
- `renderer.h`, `game.h`, `main.c`

## Building

```bash
make build-avr    # Compile for Atmega328P
make upload-avr   # Build + upload via USBasp
make clean        # Remove build/
make clean-all    # Remove build/ + _deps/
```

Memory: `-Os` (size optimization). Target: `atmega328p` @ 16MHz.

## Hardware Stack

### Display — 6× MAX7219

Six MAX7219 modules daisy-chained via SPI (MOSI → DIN of next). The controllers chain their data-in port to the next chip's `DIN`, so one SPI transaction sends 6 bytes (one byte per controller).

The 6 chips form a **48 × 8** LED matrix. The logical game buffer is **16 × 24**, so each logical row maps to 3 physical rows per MAX7219:

- Logical 16 columns are rendered using block-drawing (not per-LED)
- Display is monochrome (no colour — colour enum is used for on/off)
- SPI speed: configured in `init()`

### Joystick — Single-pin ADC

A voltage-divider circuit maps the joystick's X/Y voltage to six zones read from a single ADC channel. The renderer reads the ADC in render loop or delay:

| Direction | ADC Zone |
|---|---|
| Left | Zone 1 |
| Right | Zone 2 |
| Up | Zone 3 |
| Down | Zone 5 |
| Horizontal Hold | Zone 4 |
| Release | Zone 6 |

Implement `get_event()` by reading the ADC, comparing against thresholds, and returning `enum Event`. Hardware debounce is handled by sampling rate — rewrite at your own risk.

### Input Mappings (ADC voltage zones)

| Code | Value |
|---|---|
| 0-20 | No press (center) → `EVENT_EMPTY` |
| 21-40 | Left → `EVENT_LEFT` |
| 41-60 | Right → `EVENT_RIGHT` |
| 61-80 | Up → `EVENT_UP` |
| 81-100 | Hold → `EVENT_EMPTY` |
| 101-120 | Down → `EVENT_DOWN` |
| 120+ | Release → `EVENT_EMPTY` |

These numbers are approximate — actual values come from the calibrated voltage divider. Update `get_event()` if the circuit changes.

## Timers

### Timer0 — 8000Hz ISR

Drives sound sample timing. Provides the tick rate for sound playback. Configured as CTC mode with OCR0A prescaler.

### Timer2 — Speaker PWM

Produces audio output via speaker on pin PD3. Frequency is calculated per sample in the `play_sound()` ISR.

### Dimming

Dimming is handled by gently pulsing the LED power. PWM duty cycle maps brightness from 0 (off) to 255 (full). Adjust in `init()` when mapping MAX7219 registers.

## Adding Sounds

The same Python script works for both PC and AVR, but uses platform detection:

```python
# In generate_sound.py
if defined(__AVR__):
    # Emit SOUND_NAME_DATA[SIZE] PROGMEM = { ... }
else:
    # Emit SOUND_NAME_DATA[SIZE] = { ... }
```

### For AVR

1. Add `.wav` to `sounds/`
2. Add a Python invocation in the `build/sounds/` recipe for the AVR target
3. Include the generated header in `avr/sound.h`
4. Reference `SOUND_<NAME>` from `play_sound()`

## Programming

```bash
avrdude -c usbasp -p m328p -U flash:w:"build/main.hex":a
```

### Fuse bits

Default fuses for ATmega328P:
- LFUSE: 0xFF (8MHz internal → calibrated to 16MHz external crystal)
- HFUSE: 0xDE
- EFUSE: 0x07 (not burned by default)

Check current fuses:

```bash
avrdude -c usbasp -p m328p -U lfuse:r:-:h -U hfuse:r:-:h
```

### Hitting RESET

The AVR must be in reset state before programming. Ensure the USBasp is connected to the ICSP header and that the reset pin is pulled high → low during programming.

## Common Pitfalls

### Stack overflow

AVR has only 2KB SRAM. The 16×24 screen buffer (`uint8_t[16][24]`) is 384 bytes — meaning both `board[]` and `screen[]` eat almost half the stack. Track variable sizes carefully. Use `static` where possible to move data out of stack.

### SRAM vs Flash usage

With `-Os`, every 100 bytes of code matters. Avoid deep call chains in `renderer` functions — inline small helpers. Large arrays must use `const` so the compiler places them in flash (`PROGMEM` for data accessed at runtime).

### ADC continuity

The joystick ADC reading must happen every render frame. If drawing takes too long, the ADC ADC sampling will be too slow and unresponsive. Keep the ISR tight.

### Watchdog Timer

If using WDT don't forget to feed it. The main loop must call `watchdog_timer_fed()` or add `wdr()` call in `avr/renderer.c`.

### Power limitations

328P runs at 5V via USB. The MAX7219 array draws significant current. Ensure the power supply can handle peak draw (~50mA × 6 modules = 300mA → consider voltage regulator).

## Debug Checklist

1. Check upload succeeds with verbose avrdude:
   ```bash
   avrdude -c usbasp -p m328p -vvv -U flash:w:"build/main.hex":a
   ```
2. Verify fuse bits match expected values
3. Measure LED matrix brightness — confirm PWM duty is in range
4. ADC readings should be stable (no flickering)
5. Wait 5 seconds before reprogramming
6. Check that the hex file fits within 32KB (32KB flash)
7. Debug with incremental AVR builds:
   ```bash
   make build-avr && avrdude -c usbasp -p m328p -U flash:w:"./build/main.hex":a -v
   ```