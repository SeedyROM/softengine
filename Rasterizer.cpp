#include <math.h>
#include <stdio.h>
#include <algorithm>
#include <Rasterizer.h>

static inline int lerp(int v1, int v2, float ratio) {
	return v1 + (int)(v2 - v1) * ratio;
}

Rasterizer::Rasterizer(SDL_Renderer* renderer, int width, int height) {
	this->width = width;
	this->height = height;

	color = 0;
	screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
	pixelBuffer = new Uint32[width * height];
	depthBuffer = new int[width * height];

	setColor(255, 255, 255);
}

Rasterizer::~Rasterizer() {
	SDL_DestroyTexture(screenTexture);

	delete[] pixelBuffer;
	delete[] depthBuffer;
}

void Rasterizer::clear() {
	int bufferLength = width * height;

	std::fill(pixelBuffer, pixelBuffer + bufferLength, 0);
	std::fill(depthBuffer, depthBuffer + bufferLength, 0);
}

void Rasterizer::line(int x1, int y1, int x2, int y2) {
	int deltaX = x2 - x1;
	int deltaY = y2 - y1;
	int totalPixels = abs(deltaX) + abs(deltaY);

	for (int i = 0; i < totalPixels; i++) {
		float progress = (float)i / totalPixels;
		int x = x1 + (int)(deltaX * progress);
		int y = y1 + (int)(deltaY * progress);

		if (x < 0 || x >= width || y < 0 || y >= height) {
			continue;
		}

		bool isGoingOffScreen = (
			deltaX < 0 && x < 0 ||
			deltaX > 0 && x > width ||
			deltaY < 0 && y < 0 ||
			deltaY > 0 && y > height
		);

		if (isGoingOffScreen) {
			break;
		}

		setPixel(x, y);
	}
}

void Rasterizer::render(SDL_Renderer* renderer) {
	SDL_UpdateTexture(screenTexture, NULL, pixelBuffer, width * sizeof(Uint32));
	SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
	SDL_RenderPresent(renderer);

	clear();
}

void Rasterizer::setColor(int R, int G, int B, int A) {
	color = (A << 24) | (R << 16) | (G << 8) | B;
}

void Rasterizer::setColor(Color* color) {
	setColor(color->R, color->G, color->B, color->A);
}

void Rasterizer::triangle(int x1, int y1, int x2, int y2, int x3, int y3) {
	line(x1, y1, x2, y2);
	line(x2, y2, x3, y3);
	line(x3, y3, x1, y1);
}

/**
 * Rasterize a filled triangle with per-vertex coloration.
 */
void Rasterizer::triangle(Triangle* triangle) {
	// Determine the Y-ordering of vertices so we
	// can draw the triangle from top to bottom
	Vertex* top = triangle->vertices[0];
	Vertex* middle = triangle->vertices[1];
	Vertex* bottom = triangle->vertices[2];

	if (top->y > middle->y) {
		std::swap(top, middle);
	}

	if (middle->y > bottom->y) {
		std::swap(middle, bottom);
	}

	if (top->y > middle->y) {
		std::swap(top, middle);
	}

	// Top-to-bottom rasterization
	float topToBottomSlope = (float)(bottom->y - top->y) / (bottom->x - top->x);
	float topToMiddleSlope = (float)(middle->y - top->y) / (middle->x - top->x);
	float middleToBottomSlope = (float)(bottom->y - middle->y) / (bottom->x - middle->x);
	int triangleHeight = bottom->y - top->y;
	int triangleWidth = std::max(top->x, std::max(middle->x, bottom->x)) - std::min(top->x, std::min(middle->x, bottom->x));
	int isSmallTriangle = triangleHeight < 100 && triangleWidth < 100;
	bool hasLeftHypotenuse = topToBottomSlope > 0 ? middle->y < (topToBottomSlope * (middle->x - top->x)) : middle->y > (topToBottomSlope * (middle->x - top->x));

	for (int line = 1; line < triangleHeight; line++) {
		float heightProgress = (float)line / triangleHeight;
		int y = line + top->y;

		if (y < 0) {
			continue;
		} else if (y >= height) {
			break;
		}

		bool isPastMiddle = y > middle->y;
		float halfProgress = isPastMiddle ? (float)(y - middle->y) / (bottom->y - middle->y) : (float)(y - top->y) / (middle->y - top->y);
		int startOrigin = isPastMiddle ? middle->x : top->x;
		int startInput = isPastMiddle ? y - middle->y : line;
		float startSlope = isPastMiddle ? middleToBottomSlope : topToMiddleSlope;
		int start = startOrigin + (int)(startInput / startSlope);
		int end = top->x + (int)(line / topToBottomSlope);

		if (start > end) {
			std::swap(start, end);
		}

		int lineLength = end - start;

		// TODO:
		// 1. Clean a bunch of this garbage up
		// 4. Consider not having per-vertex coloration, but color intensity, and using a fixed step delta
		//    across each triangle scan line (avoiding the need for any repeat lerp() calls in the x loop)

		int ttbR = lerp(top->color->R, bottom->color->R, heightProgress);
		int halfR = isPastMiddle ? lerp(middle->color->R, bottom->color->R, halfProgress) : lerp(top->color->R, middle->color->R, halfProgress);
		int startR = hasLeftHypotenuse ? ttbR : halfR;
		int endR = hasLeftHypotenuse ? halfR : ttbR;

		int ttbG = lerp(top->color->G, bottom->color->G, heightProgress);
		int halfG = isPastMiddle ? lerp(middle->color->G, bottom->color->G, halfProgress) : lerp(top->color->G, middle->color->G, halfProgress);
		int startG = hasLeftHypotenuse ? ttbG : halfG;
		int endG = hasLeftHypotenuse ? halfG : ttbG;

		int ttbB = lerp(top->color->B, bottom->color->B, heightProgress);
		int halfB = isPastMiddle ? lerp(middle->color->B, bottom->color->B, halfProgress) : lerp(top->color->B, middle->color->B, halfProgress);
		int startB = hasLeftHypotenuse ? ttbB : halfB;
		int endB = hasLeftHypotenuse ? halfB : ttbB;

		for (int x = start; x < end && x < width; x++) {
			if (x < 0 || depthBuffer[y * width + x] > 0) {
				continue;
			}

			float lineProgress = (float)(x - start) / lineLength;
			int R = lerp(startR, endR, lineProgress);
			int G = lerp(startG, endG, lineProgress);
			int B = lerp(startB, endB, lineProgress);

			setColor(R, G, B);
			setPixel(x, y);
		}
	}
}

void Rasterizer::setPixel(int x, int y, int depth) {
	int index = y * width + x;

	pixelBuffer[index] = color;
	depthBuffer[index] = depth;
}
