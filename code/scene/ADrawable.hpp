#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

class ADrawable {
public:
	virtual void draw() = 0;
	virtual void erase() = 0;

};
