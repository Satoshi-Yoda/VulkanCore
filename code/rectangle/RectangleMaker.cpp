#include "RectangleMaker.h"

#include <cassert>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../core/Lava.h"

using glm::vec2;
using glm::vec3;

using namespace std;

RectangleMaker::RectangleMaker(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) : ash(ash), mountain(mountain), rocks(rocks), crater(crater), lava(lava) {}

RectangleMaker::~RectangleMaker() {}

void RectangleMaker::initRectangle() {
	int width = 100;
	int height = 100;
	vector<RectangleVertex> vertices = initQuad(width, height);

	unique_ptr<Rectangle> rectangle = make_unique<Rectangle>(ash);
	rectangle->setName("rectangle_name");
	rectangle->setWorkingData(vertices);

	rectangle->setVulkanEntities(mountain, rocks, crater, lava);
	rectangle->establish(RectangleAspect::STAGING_VERTICES);
	rectangle->establish(RectangleAspect::LIVE_VERTICES);
	rectangle->free(RectangleAspect::STAGING_VERTICES);
	rectangle->createDescriptorSet();

	lava.addRectangle(move(rectangle));
}

vector<RectangleVertex> RectangleMaker::initQuad(uint32_t w, uint32_t h) {
	float scale = 1.0f;

	int x_min = 0 - w * scale / 2;
	int x_max = x_min + w * scale;
	int y_min = 0 - h * scale / 2;
	int y_max = y_min + h * scale;

	vector<RectangleVertex> result;

	result.push_back({ { x_min, y_max }, { 0.0f, 1.0f } });
	result.push_back({ { x_max, y_max }, { 1.0f, 1.0f } });
	result.push_back({ { x_min, y_min }, { 0.0f, 0.0f } });

	result.push_back({ { x_max, y_max }, { 1.0f, 1.0f } });
	result.push_back({ { x_max, y_min }, { 1.0f, 0.0f } });
	result.push_back({ { x_min, y_min }, { 0.0f, 0.0f } });

	return result;
}
