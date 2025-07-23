run: build-sdl
	./build/sdl_tetris

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

build-sdl: sounds main.c sdl2_renderer/renderer.h sdl2_renderer/sound.h
	mkdir -p build
	gcc -Wall main.c -lSDL2 -lSDL2main -o ./build/sdl_tetris

build-avr: sounds main.c avr_renderer.h
	mkdir -p build
	avr-gcc main.c -o ./build/main.elf -mmcu=atmega328p -DF_CPU=16000000UL -Os
	avr-objcopy ./build/main.elf -O ihex ./build/main.hex

upload-avr: build-avr
	avrdude -c usbasp -p m328p -U flash:w:"./build/main.hex":a

clean:
	rm -rf build
