SDL_VERSION = 2.32.8
SDL_DIR = _deps/SDL2-$(SDL_VERSION)/x86_64-w64-mingw32
MINGW_CC = x86_64-w64-mingw32-gcc
MINGW_LD = x86_64-w64-mingw32-ld
LD_BMP = ld
SDL_URL = https://github.com/libsdl-org/SDL/releases/download/release-$(SDL_VERSION)/SDL2-devel-$(SDL_VERSION)-mingw.tar.gz

# Project sources + linker-loaded BMP
BMP_SRC = sdl2/wall.bmp
BMP_OBJ_NATIVE = build/wall_bmp_native.o
BMP_OBJ_WINDOWS = build/wall_bmp_windows.o

SOURCES_C = main.c tetris/position.c tetris/pieces.c tetris/colours.c tetris/tetris.c sdl2/renderer.c

run: build-sdl
	./build/sdl_tetris

# Ensure SDL2 development files are available
_deps/SDL2-$(SDL_VERSION):
	@mkdir -p _deps
	@echo "Downloading SDL2 development files..."
	@wget -q --show-progress "$(SDL_URL)" -O "/tmp/SDL2-devel-$(SDL_VERSION)-mingw.tar.gz"
	@echo "Extracting SDL2 development files..."
	@tar -xzf "/tmp/SDL2-devel-$(SDL_VERSION)-mingw.tar.gz" -C _deps
	@touch "_deps/SDL2-$(SDL_VERSION)" # Mark as completed

# Native: embed wall.bmp into executable via linker
$(BMP_OBJ_NATIVE): $(BMP_SRC)
	$(LD_BMP) -r -b binary -o $@ $(BMP_SRC)

# Windows: embed wall.bmp using MinGW's linker (produces compatible .o)
$(BMP_OBJ_WINDOWS): $(BMP_SRC)
	$(MINGW_LD) -r -b binary -o $@ $(BMP_SRC)

# Windows cross-compilation
build-windows: _deps/SDL2-$(SDL_VERSION) sounds $(BMP_OBJ_WINDOWS) sdl2/sound.h $(SOURCES_C)
	mkdir -p build
	$(MINGW_CC) -Wall $(SOURCES_C) $(BMP_OBJ_WINDOWS) \
		-I$(SDL_DIR)/include \
		-L$(SDL_DIR)/lib \
		-lSDL2main -lSDL2 \
		-lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lversion -luuid -lsetupapi -ladvapi32 -lshell32 -luser32 -lkernel32 \
		-static \
		-o ./build/sdl_tetris.exe

run-windows: build-windows
	wine ./build/sdl_tetris.exe

# Clean targets
# clean      - remove generated build artifacts only
# clean-all  - remove generated artifacts AND downloaded dependencies
clean:
	rm -rf build

clean-all:
	rm -rf build
	rm -rf _deps

build/sounds/dzin.h: sounds/dzin.wav
	mkdir -p build/sounds
	python3 generate_sound.py dzin

build/sounds/bump.h: sounds/bump.wav
	mkdir -p build/sounds
	python3 generate_sound.py bump

build/sounds/spoon.h: sounds/spoon.wav
	mkdir -p build/sounds
	python3 generate_sound.py spoon

build/sounds/spoon2.h: sounds/spoon2.wav
	mkdir -p build/sounds
	python3 generate_sound.py spoon2

build/sounds/click.h: sounds/click.wav
	mkdir -p build/sounds
	python3 generate_sound.py click

sounds: build/sounds/dzin.h build/sounds/bump.h build/sounds/spoon.h build/sounds/spoon2.h build/sounds/click.h

build-sdl: sounds $(BMP_OBJ_NATIVE) sdl2/sound.h $(SOURCES_C)
	mkdir -p build
	gcc -Wall $(SOURCES_C) $(BMP_OBJ_NATIVE) -lSDL2 -lSDL2main -o ./build/sdl_tetris

build-avr: sounds main.c tetris/position.c tetris/pieces.c tetris/colours.c tetris/tetris.c avr/renderer.c avr/sound.h
	mkdir -p build
	avr-gcc main.c tetris/position.c tetris/pieces.c tetris/colours.c tetris/tetris.c avr/renderer.c -o ./build/main.elf -mmcu=atmega328p -DF_CPU=16000000UL -Os
	avr-objcopy ./build/main.elf -O ihex ./build/main.hex

upload-avr: build-avr
	avrdude -c usbasp -p m328p -U flash:w:"./build/main.hex":a