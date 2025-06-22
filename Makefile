run: build
	./build/tetris

build: main.c sdl_renderer.h 
	mkdir -p build
	gcc -Wall main.c -lSDL2 -lSDL2main -o ./build/tetris

avr: main.c avr_renderer.h
	mkdir -p build
	avr-gcc main.c -o ./build/main.elf -mmcu=atmega328p -DF_CPU=16000000UL -Os
	avr-objcopy ./build/main.elf -O ihex ./build/main.hex
	avrdude -c usbasp -p m328p -U flash:w:"./build/main.hex":a

clean:
	rm -rf build
