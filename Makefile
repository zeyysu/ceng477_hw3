all:
	g++ main.cpp -o main `pkg-config --cflags --libs freetype2` -lglfw -lGLEW -framework OpenGL -std=c++11
