#include <math.h>
#include <stdio.h>
#include <algorithm>
#include <Helpers.h>
#include <Rasterizer.h>

Rasterizer::Rasterizer(SDL_Renderer* renderer, int width, int height) {
	this->width = width;
	this->height = height;

	screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
	pixelBuffer = new Uint32[width * height];
	depthBuffer = new int[width * height];

	setColor(255, 255, 255);
	clear();
}

Rasterizer::~Rasterizer() {
	SDL_DestroyTexture(screenTexture);

	delete[] pixelBuffer;
	delete[] depthBuffer;
}

void Rasterizer::clear() {
	int bufferLength = width * height;

	std::fill(pixelBuffer, pixelBuffer + bufferLength, 0);
	std::fill(depthBuffer, depthBuffer + bufferLength, INT_MAX);
}

void Rasterizer::flatTriangle(const Vertex2d& corner, const Vertex2d& left, const Vertex2d& right) {
	int triangleHeight = std::abs(left.coordinate.y - corner.coordinate.y);
	int topY = std::min(corner.coordinate.y, left.coordinate.y);
	float leftSlope = (float)triangleHeight / (left.coordinate.x - corner.coordinate.x);
	float rightSlope = (float)triangleHeight / (right.coordinate.x - corner.coordinate.x);
	bool hasFlatTop = corner.coordinate.y > left.coordinate.y;

	for (int i = 0; i < triangleHeight; i++) {
		int y = topY + i;

		if (y < 0) {
			continue;
		} else if (y >= height) {
			break;
		}

		int j = hasFlatTop ? triangleHeight - i : i;
		float progress = (float)j / triangleHeight;
		int startX = corner.coordinate.x + (int)j / leftSlope;
		int endX = corner.coordinate.x + (int)j / rightSlope;
		Color leftColor = lerp(corner.color, left.color, progress);
		Color rightColor = lerp(corner.color, right.color, progress);
		int leftDepth = lerp(corner.depth, left.depth, progress);
		int rightDepth = lerp(corner.depth, right.depth, progress);

		triangleScanLine(startX, y, endX - startX, leftColor, rightColor, leftDepth, rightDepth);
	}
}

void Rasterizer::flatBottomTriangle(const Vertex2d& top, const Vertex2d& bottomLeft, const Vertex2d& bottomRight) {
	flatTriangle(top, bottomLeft, bottomRight);
}

void Rasterizer::flatTopTriangle(const Vertex2d& topLeft, const Vertex2d& topRight, const Vertex2d& bottom) {
	flatTriangle(bottom, topLeft, topRight);
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
			(deltaX < 0 && x < 0) ||
			(deltaX > 0 && x > width) ||
			(deltaY < 0 && y < 0) ||
			(deltaY > 0 && y > height)
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

void Rasterizer::setColor(int R, int G, int B) {
	color = (255 << 24) | (R << 16) | (G << 8) | B;
}

void Rasterizer::setColor(Color* color) {
	setColor(color->R, color->G, color->B);
}

void Rasterizer::setPixel(int x, int y, int depth) {
	int index = y * width + x;

	pixelBuffer[index] = color;
	depthBuffer[index] = depth;
}

void Rasterizer::triangle(int x1, int y1, int x2, int y2, int x3, int y3) {
	line(x1, y1, x2, y2);
	line(x2, y2, x3, y3);
	line(x3, y3, x1, y1);
}

/**
 * Rasterize a filled triangle with per-vertex coloration.
 */
void Rasterizer::triangle(Triangle& triangle) {
	Vertex2d* top = &triangle.vertices[0];
	Vertex2d* middle = &triangle.vertices[1];
	Vertex2d* bottom = &triangle.vertices[2];

	if (top->coordinate.y > middle->coordinate.y) {
		std::swap(top, middle);
	}

	if (middle->coordinate.y > bottom->coordinate.y) {
		std::swap(middle, bottom);
	}

	if (top->coordinate.y > middle->coordinate.y) {
		std::swap(top, middle);
	}

	if (top->coordinate.y >= height || bottom->coordinate.y < 0) {
		return;
	}

	if (top->coordinate.y == middle->coordinate.y) {
		if (top->coordinate.x > middle->coordinate.x) {
			std::swap(top, middle);
		}

		flatTopTriangle(*top, *middle, *bottom);
	} else if (bottom->coordinate.y == middle->coordinate.y) {
		if (bottom->coordinate.x < middle->coordinate.x) {
			std::swap(bottom, middle);
		}

		flatBottomTriangle(*top, *middle, *bottom);
	} else {
		float ttbSlope = (float)(bottom->coordinate.y - top->coordinate.y) / (bottom->coordinate.x - top->coordinate.x);
		float middleYRatio = (float)(middle->coordinate.y - top->coordinate.y) / (bottom->coordinate.y - top->coordinate.y);

		Vertex2d middleOpposite;

		middleOpposite.coordinate = { top->coordinate.x + (int)((middle->coordinate.y - top->coordinate.y) / ttbSlope), middle->coordinate.y };
		middleOpposite.depth = lerp(top->depth, bottom->depth, middleYRatio);
		middleOpposite.color = lerp(top->color, bottom->color, middleYRatio);

		Vertex2d* middleLeft = middle;
		Vertex2d* middleRight = &middleOpposite;

		if (middleLeft->coordinate.x > middleRight->coordinate.x) {
			std::swap(middleLeft, middleRight);
		}

		flatBottomTriangle(*top, *middleLeft, *middleRight);
		flatTopTriangle(*middleLeft, *middleRight, *bottom);
	}
}

void Rasterizer::triangleScanLine(int x1, int y1, int lineLength, const Color& leftColor, const Color& rightColor, int leftDepth, int rightDepth) {
	if (lineLength == 0) {
		return;
	}

	for (int x = x1; x <= x1 + lineLength && x < width; x++) {
		if (x < 0) {
			continue;
		}

		float progress = (float)(x - x1) / lineLength;
		int depth = lerp(leftDepth, rightDepth, progress);

		if (depthBuffer[y1 * width + x] > depth) {
			Color* color = &lerp(leftColor, rightColor, progress);

			setColor(color);
			setPixel(x, y1, depth);
		}
	}
}