#include "RectangleMaker.h"

#include <cassert>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

using glm::vec2;
using glm::vec3;

using namespace std;

RectangleMaker::RectangleMaker(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) : ash(ash), mountain(mountain), rocks(rocks), crater(crater), lava(lava) {}

RectangleMaker::~RectangleMaker() {}

void RectangleMaker::initRectangle(int x, int y, RectangleData data) {
	int width  = round(data.size.x);
	int height = round(data.size.y);
	vector<RectangleVertex> vertices = initQuad(x, y, width, height);

	unique_ptr<Rectangle> rectangle = make_unique<Rectangle>(ash);
	rectangle->setName("rectangle_name");
	// RectangleData data;
	// sata.color = { 0.9f, 0.7f, 0.5f, 0.5f };
	// sata.size = { width, height };
	// sata.radius = 5.0f;
	// sata.step = 0.0f;
	rectangle->setWorkingData(vertices, data);

	rectangle->setVulkanEntities(mountain, rocks, crater, lava);
	rectangle->establish(RectangleAspect::STAGING_VERTICES, RectangleAspect::STAGING_DATA);
	rectangle->establish(RectangleAspect::LIVE_VERTICES, RectangleAspect::LIVE_DATA);
	rectangle->free(RectangleAspect::STAGING_VERTICES, RectangleAspect::STAGING_DATA);
	rectangle->createDescriptorSet();

	lava.addRectangle(move(rectangle));
}

vector<RectangleVertex> RectangleMaker::initQuad(int x, int y, uint32_t w, uint32_t h) {
	float scale = 1.0f;

	int x_min = x - w * scale / 2;
	int x_max = x_min + w * scale;
	int y_min = y - h * scale / 2;
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
