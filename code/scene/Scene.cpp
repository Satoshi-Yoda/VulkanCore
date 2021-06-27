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

Scene::Scene(Ash& ash, Batcher& batcher, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) : ash(ash), batcher(batcher), mountain(mountain), rocks(rocks), crater(crater), lava(lava) {}

Scene::~Scene() {}

void Scene::init() {
	vec4 color { 0.9f, 0.7f, 0.5f, 0.5f };
	vec2 size { 20, 300 };
	float radius = 4.0f;

	RectangleData rect1;
	rect1.color = color;
	rect1.size = size;
	rect1.radius = radius;
	rect1.step = 0.0f;
	rectangleMaker.initRectangle(-size.x * 1.5, 0, rect1);

	RectangleData rect2;
	rect2.color = color;
	rect2.size = size;
	rect2.radius = radius;
	rect2.step = 0.5f;
	rectangleMaker.initRectangle(0, 0, rect2);

	RectangleData rect3;
	rect3.color = color;
	rect3.size = size;
	rect3.radius = radius;
	rect3.step = 0.8f;
	rectangleMaker.initRectangle(+size.x * 1.5, 0, rect3);

	RectangleData rect4;
	rect4.color = color;
	rect4.size = size;
	rect4.radius = radius;
	rect4.step = 1.0f;
	rectangleMaker.initRectangle(+size.x * 1.5 * 2, 0, rect4);

	RectangleData rect5;
	rect5.color = color;
	rect5.size = size;
	rect5.radius = radius;
	rect5.step = 1.5f;
	rectangleMaker.initRectangle(+size.x * 1.5 * 3, 0, rect5);

	return;

	// "asteroid-s1.1"
	// printf("Lava: sprite %.0fx%.0f\n", round(width * scale), round(height * scale));

	extent_h = 800 / 2;
	extent_w = 1500 / 2;

	// full scale
	// N = 1840000;   // static
	// N = 1700000; // old stream with instances (92% from static) 780 Mb/second
	N = 200; // new stream with instances
	// N = 100000; // dynamic 10% with instances
	float percent = 0.05;
	// N = 301;

	// 0.5 scale
	// N = 5888000;
	// N = 4500000; // stream size 0.5 (76% from static) 2070 Mb/second

	// 0.25 size
	// N = 11905000;
	// N = 4875000; // stream size 0.5 (41% from static) 2240 Mb/second

	size_t count = 0.97 * sqrt(2 * N) * extent_h / extent_w;
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

	for (float x = -extent_w; x < extent_w; x += step)
	for (float y = -extent_h; y < extent_h; y += step)
	{
		Instance instance { { x + step / 2, y + step / 2 } };
		batcher.addInstance("asteroid-s3.2", instance);
	}

	// printf("Lava: %lld sprites\n", instances.size());
	// printf("Lava: stream: %.2f Mb/frame\n", static_cast<float>(instances.size() * sizeof(Instance)) / (1 << 20));
	// printf("Lava: stream: %.2f Mb/second (for 60 fps)\n", 60 * static_cast<float>(instances.size() * sizeof(Instance)) / (1 << 20));
	// printf("Lava: fillrate: %.0f Mpixels/frame\n", (instances.size()) * width * height * scale * scale / 1000000);
}

void Scene::update(double t, double dt) {
	for (auto i : updatableIndexes) {
		float add = 40 * cos(t) * dt;
		vec2 addv { add, add };
		instances[i].pos = instances[i].pos + addv;
		batcher.updateInstance("bomb.6", i, instances[i]);
	}

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
		uniform_int_distribution<int> xDistribution { -extent_w, extent_w };
		uniform_int_distribution<int> yDistribution { -extent_h, extent_h };

		Instance instance { { xDistribution(random), yDistribution(random) } };
		size_t index = batcher.addInstance("bomb.6", instance);
		instances[index] = instance;

		nextChange = nextChange + 1.0/60.0;
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
