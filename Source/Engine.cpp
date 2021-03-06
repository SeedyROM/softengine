#define _CRT_SECURE_NO_WARNINGS

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <Objects.h>
#include <Rasterizer.h>
#include <Helpers.h>
#include <Engine.h>

Engine::Engine(int width, int height, Uint32 flags) {
	SDL_Init(SDL_INIT_EVERYTHING);

	window = SDL_CreateWindow(
		"HEY ZACK",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width, height,
		SDL_WINDOW_SHOWN
	);

	renderer = SDL_CreateRenderer(window, -1, flags & DEBUG_DRAWTIME ? 0 : SDL_RENDERER_PRESENTVSYNC);
	rasterizer = new Rasterizer(renderer, width, height);

	this->width = width;
	this->height = height;
	this->flags = flags;
}

Engine::~Engine() {
	objects.clear();
	delete rasterizer;

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Engine::addObject(Object* object) {
	objects.push_back(object);
}

void Engine::delay(int ms) {
	int startTime = SDL_GetTicks();

	if (ms > 0) {
		while ((SDL_GetTicks() - startTime) < ms) {
			SDL_Delay(1);
		}
	}
}

void Engine::draw() {
	RotationMatrix rotationMatrix = RotationMatrix::calculate(camera.rotation);
	int fovScalar = 500 * (360 / camera.fov);

	for (int o = 0; o < objects.size(); o++) {
		Object* object = objects.at(o);
		Vec3 relativeObjectPosition = object->position - camera.position;

		object->forEachPolygon([=](const Polygon& polygon) {
			Triangle triangle;
			bool isInView = false;

			for (int i = 0; i < 3; i++) {
				Vec3 vertex = rotationMatrix * (relativeObjectPosition + polygon.vertices[i]->vector);
				Vec3 unitVertex = vertex.unit();
				float distortionCorrectedZ = unitVertex.z * std::abs(std::cos(unitVertex.x));
				int x = (int)(fovScalar * unitVertex.x / (1 + unitVertex.z) + width / 2);
				int y = (int)(fovScalar * -unitVertex.y / (1 + distortionCorrectedZ) + height / 2);

				if (!isInView && vertex.z > 0) {
					isInView = true;
				}

				triangle.createVertex(i, x, y, (int)vertex.z, polygon.vertices[i]->color);
			}

			if (isInView) {
				if (flags & SHOW_WIREFRAME) {
					rasterizer->setColor(255, 255, 255);

					rasterizer->triangle(
						triangle.vertices[0].coordinate.x, triangle.vertices[0].coordinate.y,
						triangle.vertices[1].coordinate.x, triangle.vertices[1].coordinate.y,
						triangle.vertices[2].coordinate.x, triangle.vertices[2].coordinate.y
					);
				} else {
					rasterizer->triangle(triangle);
				}
			}
		});
	}

	rasterizer->render(renderer);
}

int Engine::getPolygonCount() {
	int total = 0;

	for (int o = 0; o < objects.size(); o++) {
		total += objects.at(o)->getPolygonCount();
	}

	return total;
}

void Engine::handleEvent(const SDL_Event& event) {
	switch (event.type) {
		case SDL_KEYDOWN:
			handleKeyDown(event.key.keysym.sym);
			break;
		case SDL_KEYUP:
			handleKeyUp(event.key.keysym.sym);
			break;
		case SDL_MOUSEMOTION:
			handleMouseMotionEvent(event.motion);
			break;
	}
}

void Engine::handleKeyDown(const SDL_Keycode& code) {
	switch (code) {
		case SDLK_w: movement.z = 1; break;
		case SDLK_s: movement.z = -1; break;
		case SDLK_a: movement.x = -1; break;
		case SDLK_d: movement.x = 1; break;
	}
}

void Engine::handleKeyUp(const SDL_Keycode& code) {
	switch (code) {
		case SDLK_w: movement.z = 0; break;
		case SDLK_s: movement.z = 0; break;
		case SDLK_a: movement.x = 0; break;
		case SDLK_d: movement.x = 0; break;
	}
}

void Engine::handleMouseMotionEvent(const SDL_MouseMotionEvent& event) {
	int mx = event.x - width / 2;
	int my = event.y - height / 2;
	int xDelta = lastMouseCoordinate.x - event.x;
	int yDelta = lastMouseCoordinate.y - event.y;
	float deltaFactor = 1.0f / sqrt(mx * mx + my * my);

	camera.rotation.y += (float)xDelta * deltaFactor;
	camera.rotation.x += (float)yDelta * deltaFactor;

	lastMouseCoordinate.x = event.x;
	lastMouseCoordinate.y = event.y;
}

void Engine::run() {
	int lastStartTime;
	bool isRunning = true;

	while (isRunning) {
		lastStartTime = SDL_GetTicks();

		updateMovement();
		draw();

		int delta = SDL_GetTicks() - lastStartTime;

		if (flags & DEBUG_DRAWTIME) {
			if (delta < 17) {
				delay(17 - delta);
			} else {
				printf("[DRAW TIME WARNING] ");
			}
			
			printf("Unlocked delta: %d\n", delta);
		}

		int fullDelta = SDL_GetTicks() - lastStartTime;
		char title[100];

		sprintf(title, "Objects: %d, Polygons: %d, FPS: %dfps, Unlocked delta: %dms", objects.size(), getPolygonCount(), (int)round(60 * 17 / fullDelta), delta);

		SDL_SetWindowTitle(window, title);

		SDL_Event event;
		float speed = 5;

		while (SDL_PollEvent(&event)) {
			handleEvent(event);

			if (event.type == SDL_QUIT) {
				isRunning = false;
				break;
			}
		}
	}
}

void Engine::updateMovement() {
	float sy = std::sin(camera.rotation.y);
	float cy = std::cos(camera.rotation.y);

	float xDelta = movement.x * cy - movement.z * sy;
	float zDelta = movement.z * cy + movement.x * sy;

	camera.position.x += MOVEMENT_SPEED * xDelta;
	camera.position.z += MOVEMENT_SPEED * zDelta;
}