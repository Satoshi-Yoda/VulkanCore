#include "RectangleMaker.h"

#include <cassert>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

using glm::vec2;
using glm::vec3;

using namespace std;

RectangleMaker::RectangleMaker(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) : ash(ash), mountain(mountain), rocks(rocks), crater(crater), lava(lava) {}

RectangleMaker::~RectangleMaker() {}

void RectangleMaker::initRectangle() {
	int width = 250;
	int height = 250;
	vector<RectangleVertex> vertices = initQuad(width, height);

	unique_ptr<Rectangle> rectangle = make_unique<Rectangle>(ash);
	rectangle->setName("rectangle_name");
	RectangleData rectangleData;
	rectangleData.color = { 0.9f, 0.7f, 0.5f, 0.5f };
	rectangleData.size = { width, height };
	rectangleData.radius = 50.0f;
	rectangle->setWorkingData(vertices, rectangleData);

	rectangle->setVulkanEntities(mountain, rocks, crater, lava);
	rectangle->establish(RectangleAspect::STAGING_VERTICES, RectangleAspect::STAGING_DATA);
	rectangle->establish(RectangleAspect::LIVE_VERTICES, RectangleAspect::LIVE_DATA);
	rectangle->free(RectangleAspect::STAGING_VERTICES, RectangleAspect::STAGING_DATA);
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

	result.push_back({ { x_min, y_max }, { 0, y_max - y_min } });
	result.push_back({ { x_max, y_max }, { x_max - x_min, y_max - y_min } });
	result.push_back({ { x_min, y_min }, { 0, 0 } });

	result.push_back({ { x_max, y_max }, { x_max - x_min, y_max - y_min } });
	result.push_back({ { x_max, y_min }, { x_max - x_min, 0 } });
	result.push_back({ { x_min, y_min }, { 0, 0 } });

	return result;
}
