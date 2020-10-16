#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>

// #define GLFW_INCLUDE_VULKAN
// #include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../engine/Lava.h"
#include "../engine/Tectonic.h"

#include "Sprite.h"

using std::vector;
using std::string;
using glm::vec2;

class Scene {
public:
	Scene();
	~Scene();

	void load();
	void establish(Lava &lava, Tectonic &tectonic);

private:
	const string MODEL_PATH = "models/viking_room.obj";
	const string TEXTURE_PATH = "pictures/viking_room.png";

	vector<Sprite> sprites;

	vector<Vertex> vertices;

	int width, height;
	void* pixels;

	void loadSquare();
	void loadVikingRoomModel();
	void move(vec2 shift);
	void scale(float value);
	void addSprite(int x, int y, int w, int h);
};

#endif