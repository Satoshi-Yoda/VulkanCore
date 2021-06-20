#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../batch/Batcher.h"
#include "../batch/BatchLayout.h"
#include "../core/Lava.h"
#include "../core/Tectonic.h"
#include "../rectangle/Rectangle.h"
#include "../rectangle/RectangleLayout.h"

using glm::vec2;
using std::mt19937_64;
using std::string;
using std::uniform_real_distribution;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

class Scene {
public:
	Scene(Ash& ash, Batcher& batcher, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava);
	~Scene();

	void init();
	void update(double t, double dt);
	size_t sprites();

private:
	Ash& ash;
	Batcher& batcher;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;
	Lava& lava;

	unordered_map<size_t, Instance> instances;
	vector<size_t> updatableIndexes;

	mt19937_64 random {};
	uniform_real_distribution<double> distribution { 0.0, 1.0 };
	int N;
	int extent_w, extent_h;
	double nextChange = 0.0f;

	unique_ptr<Rectangle> initRectangle();
	vector<Vertex> initQuad(uint32_t w, uint32_t h);
};
