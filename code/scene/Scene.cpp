#include "Scene.h"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "../engine/Lava.h"
#include "../engine/Tectonic.h"
#include "../utils/Loader.h"
// #include "../utils/utils.h"

using namespace std;

Scene::Scene() {}

Scene::~Scene() {}

void Scene::load() {
	// loadTexture(TEXTURE_PATH.c_str(), pixels, &width, &height);
	// TODO do some checking here that pixels != nullptr, or else: "Failed to load texture image!"
	// TODO maybe some wrapper loader can return stub 1x1 image in that case

	// loadTexture("pictures/tile.png", pixels, &width, &height);
}

void Scene::move(vec2 shift) {
	for (auto& v : vertices) {
		v.pos[0] += shift[0];
		v.pos[1] += shift[1];
	}
}

void Scene::scale(float value) {
	for (auto& v : vertices) {
		v.pos *= value;
	}
}

void Scene::addSprite(int x, int y, int w, int h) {
	float scale = 1.0f;

	int x_min = x - w * scale / 2;
	int x_max = x_min + w * scale;
	int y_min = y - h * scale / 2;
	int y_max = y_min + h * scale;

	vertices.push_back({ { x_min, y_max }, { 0.0f, 1.0f } });
	vertices.push_back({ { x_min, y_min }, { 0.0f, 0.0f } });
	vertices.push_back({ { x_max, y_max }, { 1.0f, 1.0f } });

	vertices.push_back({ { x_max, y_max }, { 1.0f, 1.0f } });
	vertices.push_back({ { x_min, y_min }, { 0.0f, 0.0f } });
	vertices.push_back({ { x_max, y_min }, { 1.0f, 0.0f } });
}

void Scene::establish(Lava &lava) {
	loadTexture("pictures/tile.png", pixels, &width, &height);

	int extent_h = 800 / 2;
	int extent_w = 1500 / 2;
	int N = 1840000;   // static
	// int N = 419560; // stream
	// int N = 301;
	int count = 0.97 * sqrt(2 * N) * extent_h / extent_w;
	float step = 2.0f * extent_h / count;

	for (float x = -extent_w; x < extent_w; x += step)
	for (float y = -extent_h; y < extent_h; y += step)
	{
		addSprite(x, y, width, height);
	}

	printf("Lava: %d = %d faces\n", count * count * 2 * extent_w / extent_h, vertices.size() / 3);
	printf("Lava: %d sprites\n", vertices.size() / 6);
	printf("Lava: %d Mpixels / frame\n", (vertices.size() / 6) * width * height / 1000000);

	vector<Vertex> vertices2;
	vertices2.resize(vertices.size());
	size_t size = sizeof(Vertex) * vertices.size();

	auto start = chrono::high_resolution_clock::now();
	memcpy(vertices2.data(), vertices.data(), size);
	auto finish = chrono::high_resolution_clock::now();
	auto delay = chrono::duration_cast<chrono::duration<double>>(finish - start).count();
	float speed = static_cast<float>(size) / (1 << 30) / delay;
	printf("Copied v2v %d MB in %.3fs at %.2f GB/s\n", size / (1 << 20), delay, speed);

	lavaObjectId = lava.addObject(vertices2, width, height, pixels);

	printf("Lava: %d draw calls\n", lava.texturesCount());

	freeTexture(pixels); // TODO move somewhere, maybe
	vertices.clear();
	vertices.shrink_to_fit();
}

void Scene::update(Lava &lava, double t) {
	// for (auto& v : vertices) {
	// 	vec2 add { 0, 1 * sin(t) };
	// 	v.pos = v.pos + add;
	// }
	// lava.updateVertexBuffer(lavaObjectId, vertices);
}
