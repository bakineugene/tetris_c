run: build
	./build/tetris

build: main.c sdl_renderer.h 
	mkdir -p build
	gcc main.c -lSDL2 -lSDL2main -o ./build/tetris

clean:
	rm -rf build
