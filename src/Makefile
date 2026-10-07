.PHONY: reglyph

reglyph:
	g++ -g -O0 main.cpp ../external/glad/src/gl.c \
		-I../external/glad/include \
		-I/usr/include/freetype2 \
		-lglfw -lGL -lfreetype \
		-o reglyph

release:
	g++ -g -O3 main.cpp ../external/glad/src/gl.c \
		-I../external/glad/include \
		-I/usr/include/freetype2 \
		-lglfw -lGL -lfreetype \
		-o reglyph
