#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../core/Lava.h"
#include "../core/Tectonic.h"
#include "../engine/Batcher.h"

using std::vector;
using std::string;
using glm::vec2;

class Scene {
public:
	Scene(Batcher& batcher);
	~Scene();

	void init();
	void update(double t, double dt);

private:
	Batcher& batcher;

	vector<Instance> instances;
	vector<size_t> updatableIndexes;
};

#endif