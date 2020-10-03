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

using std::vector;
using std::string;

class Scene {
public:
	Scene();
	~Scene();

	void loadSquare();
	void loadVikingRoomModel();
	void loadVikingRoomTexture();
	void establish(Lava &lava, Tectonic &tectonic);

private:
	const string MODEL_PATH = "models/viking_room.obj";
	const string TEXTURE_PATH = "pictures/viking_room.png";

	vector<Vertex> vertices;
	vector<uint32_t> indices;

	int width, height, channels;
	void* pixels;
};

#endif