#include "GraphicMaker.h"

#include <cassert>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

using glm::vec2;
using glm::vec3;

using namespace std;

GraphicMaker::GraphicMaker(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) : ash(ash), mountain(mountain), rocks(rocks), crater(crater), lava(lava) {}

GraphicMaker::~GraphicMaker() {}

void GraphicMaker::initGraphic(int x, int y, GraphicData data) {
	int width  = round(data.size.x);
	int height = round(data.size.y);
	vector<GraphicVertex> vertices = initQuad(x, y, width, height);

	unique_ptr<Graphic> graphic = make_unique<Graphic>(ash);
	graphic->setName("graphic_name");
	graphic->setWorkingData(vertices, data);

	graphic->setVulkanEntities(mountain, rocks, crater, lava);
	graphic->establish(GraphicAspect::STAGING_VERTICES, GraphicAspect::STAGING_DATA);
	graphic->establish(GraphicAspect::LIVE_VERTICES, GraphicAspect::LIVE_DATA);
	graphic->free(GraphicAspect::STAGING_VERTICES, GraphicAspect::STAGING_DATA);
	graphic->createDescriptorSet();

	lava.addGraphic(move(graphic));
}

vector<GraphicVertex> GraphicMaker::initQuad(int x, int y, uint32_t w, uint32_t h) {
	int x_min = x - w / 2;
	int x_max = x_min + w;
	int y_min = y - h / 2;
	int y_max = y_min + h;

	vector<GraphicVertex> result;

	result.push_back({ { x_min, y_max }, { 0 - 0.5, h + 0.5 } });
	result.push_back({ { x_max, y_max }, { w + 0.5, h + 0.5 } });
	result.push_back({ { x_min, y_min }, { 0 - 0.5, 0 - 0.5 } });

	result.push_back({ { x_max, y_max }, { w + 0.5, h + 0.5 } });
	result.push_back({ { x_max, y_min }, { w + 0.5, 0 - 0.5 } });
	result.push_back({ { x_min, y_min }, { 0 - 0.5, 0 - 0.5 } });

	return result;
}
