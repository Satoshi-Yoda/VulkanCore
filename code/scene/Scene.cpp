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
		{ {-0.7f, -0.7f, 0.0f }, { 1.9f, 1.9f, 1.9f }, { 0.0f, 0.0f } },
		{ {-0.7f,  0.7f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
		{ { 0.7f, -0.7f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 0.7f,  0.7f, 0.0f }, { 0.1f, 0.1f, 0.1f }, { 1.0f, 1.0f } },
	};
}

void Scene::load() {
	loadVikingRoomModel();
	loadVikingRoomTexture();
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

	printf("Loaded %s in %.3fs\n", filename.data(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
}

void Scene::loadVikingRoomTexture() {
	loadTexture(TEXTURE_PATH.c_str(), pixels, &width, &height);
	// TODO do some checking here that pixels != nullptr, or else: "Failed to load texture image!"
	// TODO maybe some wrapper loader can return stub 1x1 image in that case
}

void Scene::establish(Lava &lava, Tectonic &tectonic) {
	lava.establishVertexBuffer(vertices);
	lava.establishTexture(width, height, pixels);
	tectonic.createDescriptorSets();

	freeTexture(pixels); // TODO move somewhere, maybe
}
