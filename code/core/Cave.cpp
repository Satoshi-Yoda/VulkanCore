#include "cave.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

using glm::vec2;
using glm::vec3;

using namespace std;

Cave::Cave(vector<Vertex> vertices, int width, int height, void* pixels) : vertices(vertices), width(width), height(height), pixels(pixels) { }

Cave::~Cave() {
	// TODO free all allocated things
}

void Cave::establishAspects(CaveAspects aspects) {
	// TODO implement
	this->aspects = aspects;
}

bool Cave::canBeDrawn() {
	return static_cast<bool>(aspects & CaveAspects::LIVE_VERTICES)
	    && static_cast<bool>(aspects & CaveAspects::LIVE_INSTANCES)
	    && static_cast<bool>(aspects & CaveAspects::LIVE_TEXTURE);
}
