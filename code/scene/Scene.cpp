#include "Scene.h"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "../engine/Lava.h"
#include "../engine/Tectonic.h"
#include "../utils/Loader.h"

using namespace std;

Scene::Scene() {}

Scene::~Scene() {}

void Scene::loadSquare() {
	vertices = {
		{ {-1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
		{ {-1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
	};
}

void Scene::load() {
	// loadVikingRoomModel();
	// loadTexture(TEXTURE_PATH.c_str(), pixels, &width, &height);
	// TODO do some checking here that pixels != nullptr, or else: "Failed to load texture image!"
	// TODO maybe some wrapper loader can return stub 1x1 image in that case

	// loadSquare();
	// loadTexture("pictures/tile.png", pixels, &width, &height);
}

void Scene::loadVikingRoomModel() {
	auto start = chrono::high_resolution_clock::now();

	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string warn, err;

	string filename = MODEL_PATH;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.data())) {
		throw runtime_error(warn + err);
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertices.push_back(vertex);
		}
	}

	printf("Loaded %s with %d vertices in %.3fs\n", filename.data(), vertices.size(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
}

void Scene::move(vec2 shift) {
	for (auto& v : vertices) {
		v.pos[0] += shift[0];
		v.pos[1] += shift[1];
	}
}

void Scene::scale(float value) {
	for (auto& v : vertices) {
		v.pos *= value;
	}
}

void Scene::establish(Lava &lava, Tectonic &tectonic) {
	// loadSquare();
	loadTexture("pictures/tile.png", pixels, &width, &height);

	int count = 32;
	float extent = 1.0f;
	float step = 2 * extent / count;

	for (float x = -extent; x < extent; x += step)
	for (float y = -extent; y < extent; y += step)
	{
		loadSquare();
		scale(0.5f * step);
		move({ x, y });
		lava.addObject(vertices, width, height, pixels);
	}

	printf("Lava: %d draw calls\n", lava.texturesCount());

	freeTexture(pixels); // TODO move somewhere, maybe
}
