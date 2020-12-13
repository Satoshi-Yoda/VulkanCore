#include "Scene.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "../core/Lava.h"
#include "../utils/Loader.h"

using namespace std;

Scene::Scene(Batcher& batcher) : batcher(batcher) {}

Scene::~Scene() {}

void Scene::init() {
	// "asteroid-s1.1"

	// printf("Lava: sprite %.0fx%.0f\n", round(width * scale), round(height * scale));

	extent_h = 800 / 2;
	extent_w = 1500 / 2;

	// full scale
	// int N = 1840000;   // static
	// int N = 1700000; // old stream with instances (92% from static) 780 Mb/second
	int N = 200000; // new stream with instances
	// int N = 100000; // dynamic 10% with instances
	float percent = 0.01;
	// N = 301;

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
		Instance instance { { x, y } };
		size_t index = batcher.addInstance("bomb.6", instance);
		instances[index] = instance;
		if (distribution(random) < percent) {
			updatableIndexes.push_back(index);
		}
	}

	// printf("Lava: %lld sprites\n", instances.size());
	// printf("Lava: stream: %.2f Mb/frame\n", static_cast<float>(instances.size() * sizeof(Instance)) / (1 << 20));
	// printf("Lava: stream: %.2f Mb/second (for 60 fps)\n", 60 * static_cast<float>(instances.size() * sizeof(Instance)) / (1 << 20));
	// printf("Lava: fillrate: %.0f Mpixels/frame\n", (instances.size()) * width * height * scale * scale / 1000000);

	// vector<Vertex> vertices2;
	// vertices2.resize(vertices.size());
	// size_t size = sizeof(Vertex) * vertices.size();

	// auto start = chrono::high_resolution_clock::now();
	// memcpy(vertices2.data(), vertices.data(), size);
	// auto finish = chrono::high_resolution_clock::now();
	// auto delay = chrono::duration_cast<chrono::duration<double>>(finish - start).count();
	// float speed = static_cast<float>(size) / (1 << 30) / delay;
	// printf("Copied v2v %d MB in %.3fs at %.2f GB/s\n", size / (1 << 20), delay, speed);

/*
	BatchCreateData data {};
	data.pixels = pixels;
	data.width  = width;
	data.height = height;
	data.vertices = vertices;
	data.instances = instances;
	// lavaObjectId = lava.addObject(vertices, instances, width, height, pixels);
	lavaObjectId = lava.addBatch(data);

	printf("Lava: %lld draw calls\n", lava.batchData.size());

	freeTexture(pixels); // TODO move somewhere, maybe
	vertices.clear();
	vertices.shrink_to_fit();
	// instances.clear();
	// instances.shrink_to_fit();
*/

	// for (int i = 0; i < N * percent; i++) {
	// 	size_t index = distribution(random);
	// 	updatableIndexes.push_back(index);
	// }
}

void Scene::update(double t, double dt) {
	for (auto i : updatableIndexes) {
		float add = 40 * cos(t) * dt;
		vec2 addv { add, add };
		instances[i].pos = instances[i].pos + addv;
		batcher.updateInstance("bomb.6", i, instances[i]);
	}

	

	// if (distribution(random) <  500 * dt) {
	if (t >= nextChange) {
		uniform_int_distribution<size_t> indexDistribution { 0, static_cast<size_t>(N * 2.5) };

		for (int i = 0; i < 100000; i++) {
			size_t index = indexDistribution(random);

			if (instances.find(index) != instances.end())
			if (find(updatableIndexes.begin(), updatableIndexes.end(), index) == updatableIndexes.end())
			{
				instances.erase(index);
				batcher.removeInstance("bomb.6", index);
				break;
			}
		}
	}

	if (t >= nextChange) {
	// if (distribution(random) <  500 * dt) {
		uniform_int_distribution<int> xDistribution { -extent_w, extent_w };
		uniform_int_distribution<int> yDistribution { -extent_h, extent_h };

		Instance instance { { xDistribution(random), yDistribution(random) } };
		size_t index = batcher.addInstance("bomb.6", instance);
		instances[index] = instance;

		nextChange = nextChange + 0.01;
	}

	// TODO
	// so, if changed about 1% or less, then updateInstances(), else updateInstanceBuffer()
	// and updateInstances() loads CPU, but updateInstanceBuffer() loads PCI-E,
	// lava.updateInstances(lavaObjectId, instances, updatableIndexes);
	// lava.updateInstanceBuffer(lavaObjectId, instances);
}

size_t Scene::sprites() {
	return instances.size();
}
