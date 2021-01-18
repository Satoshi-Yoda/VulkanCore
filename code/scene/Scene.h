#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../core/Lava.h"
#include "../core/Tectonic.h"
#include "../engine/Batcher.h"

using std::string;
using std::unordered_map;
using std::vector;
using std::mt19937_64;
using std::uniform_real_distribution;
using glm::vec2;

class Scene {
public:
	Scene(Batcher& batcher);
	~Scene();

	void init();
	void update(double t, double dt);
	size_t sprites();

private:
	Batcher& batcher;

	unordered_map<size_t, Instance> instances;
	vector<size_t> updatableIndexes;

	mt19937_64 random {};
	uniform_real_distribution<double> distribution { 0.0, 1.0 };
	int N;
	int extent_w, extent_h;
	double nextChange = 0.0f;
};
