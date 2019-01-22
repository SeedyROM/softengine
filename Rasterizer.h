#pragma once

#include <SDL.h>

struct Color {
	int R = 255;
	int G = 255;
	int B = 255;
	int A = 255;
};

struct Vertex2D {
	int x = 0;
	int y = 0;
	Color color;
};

struct Polygon2D {
	Vertex2D vertices[3];

	void setVertex(int index, int x, int y, int R, int G, int B, int A = 255) {
		Vertex2D vertex = vertices[index];

		vertex.x = x;
		vertex.y = y;

		vertex.color.R = R;
		vertex.color.G = G;
		vertex.color.B = B;
		vertex.color.A = A;
	}
};

struct Optimizations {
};

class Rasterizer {
	public:
		Rasterizer(SDL_Renderer* renderer, int width, int height);
		~Rasterizer();
		Optimizations optimizations;
		void line(int x1, int y1, int x2, int y2);
		void render(SDL_Renderer* renderer);
		void setColor(int R, int G, int B, int A = 255);
		void setColor(Color* color);
		void triangle(int x1, int y1, int x2, int y2, int x3, int y3);
		void triangle(Polygon2D* triangle);
	private:
		SDL_Texture* screenTexture;
		Uint32* pixelBuffer;
		int* depthBuffer;
		long int color;
		int width;
		int height;
		void clear();
		void setPixel(int x, int y, int depth = 1);
};