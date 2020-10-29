#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../core/Lava.h"
#include "../core/Tectonic.h"

using std::vector;
using std::string;
using glm::vec2;

class Scene {
public:
	Scene();
	~Scene();

	void load();
	void establish(Lava &lava);
	void update(Lava &lava, double t, double dt);

private:
	vector<Vertex> vertices;
	vector<Instance> instances;
	vector<size_t> updatableIndexes;

	int width, height;
	void* pixels;

	size_t lavaObjectId;

	void move(vec2 shift);
	void scale(float value);
	void initRect(int x, int y, int w, int h, float scale);
	void addInstance(int x, int y);
};

#endif