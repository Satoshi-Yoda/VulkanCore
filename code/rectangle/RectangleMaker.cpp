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
	rectangle->setWorkingData(vertices, data);

	rectangle->setVulkanEntities(mountain, rocks, crater, lava);
	rectangle->establish(RectangleAspect::STAGING_VERTICES, RectangleAspect::STAGING_DATA);
	rectangle->establish(RectangleAspect::LIVE_VERTICES, RectangleAspect::LIVE_DATA);
	rectangle->free(RectangleAspect::STAGING_VERTICES, RectangleAspect::STAGING_DATA);
	rectangle->createDescriptorSet();

	lava.addRectangle(move(rectangle));
}

vector<RectangleVertex> RectangleMaker::initQuad(int x, int y, uint32_t w, uint32_t h) {
	int x_min = x - w / 2;
	int x_max = x_min + w;
	int y_min = y - h / 2;
	int y_max = y_min + h;

	vector<RectangleVertex> result;

	// h--;
	// w--;

	result.push_back({ { x_min, y_max }, { 0 - 0.5, h + 0.5 } });
	result.push_back({ { x_max, y_max }, { w + 0.5, h + 0.5 } });
	result.push_back({ { x_min, y_min }, { 0 - 0.5, 0 - 0.5 } });

	result.push_back({ { x_max, y_max }, { w + 0.5, h + 0.5 } });
	result.push_back({ { x_max, y_min }, { w + 0.5, 0 - 0.5 } });
	result.push_back({ { x_min, y_min }, { 0 - 0.5, 0 - 0.5 } });

	return result;
}
