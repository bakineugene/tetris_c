run: build
	./build/tetris

build: 
	mkdir -p build
	gcc main.c -lSDL2 -lSDL2main -o ./build/tetris

clean:
	rm -rf build
