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

void Scene::initRect(int x, int y, int w, int h, float scale) {
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

void Scene::addInstance(int x, int y) {
	instances.push_back({ { x, y } });
}

void Scene::establish(Lava &lava) {
	loadTexture("pictures/tile.png", pixels, &width, &height);

	float scale = 1.0f;
	initRect(0, 0, width, height, scale);
	printf("Lava: sprite %.0fx%.0f\n", round(width * scale), round(height * scale));

	int extent_h = 800 / 2;
	int extent_w = 1500 / 2;

	// full scale
	// int N = 1840000;   // static
	// int N = 419560; // stream with vertices
	// int N = 1700000; // stream with instances (92% from static) 780 Mb/second
	int N = 301;

	// 0.5 scale
	// int N = 5888000;
	// int N = 4500000; // stream size 0.5 (76% from static) 2070 Mb/second

	// 0.25 size
	// int N = 11905000;
	// int N = 4875000; // stream size 0.5 (41% from static) 2240 Mb/second

	int count = 0.97 * sqrt(2 * N) * extent_h / extent_w;
	float step = 2.0f * extent_h / count;

	for (float x = -extent_w; x < extent_w; x += step)
	for (float y = -extent_h; y < extent_h; y += step)
	{
		addInstance(x, y);
	}

	printf("Lava: %d vertices\n", vertices.size());
	printf("Lava: %d sprites\n", instances.size());
	printf("Lava: stream: %.2f Mb/frame\n", static_cast<float>(instances.size() * sizeof(Instance)) / (1 << 20));
	printf("Lava: stream: %.2f Mb/second (for 60 fps)\n", 60 * static_cast<float>(instances.size() * sizeof(Instance)) / (1 << 20));
	printf("Lava: fillrate: %.0f Mpixels/frame\n", (instances.size()) * width * height * scale * scale / 1000000);

	// vector<Vertex> vertices2;
	// vertices2.resize(vertices.size());
	// size_t size = sizeof(Vertex) * vertices.size();

	// auto start = chrono::high_resolution_clock::now();
	// memcpy(vertices2.data(), vertices.data(), size);
	// auto finish = chrono::high_resolution_clock::now();
	// auto delay = chrono::duration_cast<chrono::duration<double>>(finish - start).count();
	// float speed = static_cast<float>(size) / (1 << 30) / delay;
	// printf("Copied v2v %d MB in %.3fs at %.2f GB/s\n", size / (1 << 20), delay, speed);

	lavaObjectId = lava.addObject(vertices, instances, width, height, pixels);

	printf("Lava: %d draw calls\n", lava.texturesCount());

	freeTexture(pixels); // TODO move somewhere, maybe
	vertices.clear();
	vertices.shrink_to_fit();
	// instances.clear();
	// instances.shrink_to_fit();
}

void Scene::update(Lava &lava, double t, double dt) {
	// for (auto& s : instances) {
	// 	vec2 add { 0, 40 * cos(t) * dt };
	// 	s.pos = s.pos + add;
	// }
	// lava.updateInstanceBuffer(lavaObjectId, instances);

	vector<uint32_t> indexes;
	indexes.push_back(2);
	indexes.push_back(5);

	for (auto i : indexes) {
		vec2 add { 0, 40 * cos(t) * dt };
		instances[i].pos = instances[i].pos + add;
	}

	lava.updateInstances(lavaObjectId, instances, indexes);
}
