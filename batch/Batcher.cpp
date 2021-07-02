#include "Batcher.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include "../state/flag_group.hpp"
#include "../utils/Loader.h"

using namespace std;

Batcher::Batcher(Ash& ash, Team& team): ash(ash), team(team) {}

Batcher::~Batcher() {}

void Batcher::loadFolder(string folder) {
	auto start = chrono::high_resolution_clock::now();

	size_t i = 0;
	for (const auto& entry : filesystem::recursive_directory_iterator(folder)) {
		auto path = entry.path();
		if (path.extension().string() == ".png") {
			size_t index = i++;
			team.task(ST_CPU, [path, index, this]{
				string name = path.stem().string();

				void* pixels;
				int width, height;
				loadTexture(path.string(), pixels, &width, &height);

				vector<Vertex> vertices = initQuad(width, height);

				unique_ptr<Batch> batch = make_unique<Batch>(ash);
				batch->setName(name);
				batch->setWorkingData(vertices, width, height, pixels);

				putMutex.lock();
					batches[name] = move(batch);
					indexes[name] = index;
				putMutex.unlock();
			});
		}
	}

	printf("Loaded %lld files: %s/*.png in %.3fs\n", i, folder.data(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
}

vector<Vertex> Batcher::initQuad(uint32_t w, uint32_t h) {
	float scale = 1.0f;

	int x_min = 0 - w * scale / 2;
	int x_max = x_min + w * scale;
	int y_min = 0 - h * scale / 2;
	int y_max = y_min + h * scale;

	vector<Vertex> result;

	result.push_back({ { x_min, y_max }, { 0.0f, 1.0f } });
	result.push_back({ { x_max, y_max }, { 1.0f, 1.0f } });
	result.push_back({ { x_min, y_min }, { 0.0f, 0.0f } });

	result.push_back({ { x_max, y_max }, { 1.0f, 1.0f } });
	result.push_back({ { x_max, y_min }, { 1.0f, 0.0f } });
	result.push_back({ { x_min, y_min }, { 0.0f, 0.0f } });

	return result;
}

// void Batcher::addSampleInstance(string name) {
// 	random_device random {};
// 	int border = 200;
// 	uniform_int_distribution<int> x { - (1600 - border) / 2, (1600 - border) / 2 };
// 	uniform_int_distribution<int> y { -  (900 - border) / 2,  (900 - border) / 2 };

// 	addInstance(name, { { x(random), y(random) } });
// }



// void Batcher::establish(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
// 	this->ash = &ash;
// 	this->lava = &lava;

// 	auto start = chrono::high_resolution_clock::now();

// 	for (auto& it : batches) {
// 		auto& batch = it.second;
// 		batch->setVulkanEntities(ash, mountain, rocks, crater);
// 		batch->establish(BatchAspect::STAGING_VERTICES, BatchAspect::STAGING_INSTANCES, BatchAspect::STAGING_TEXTURE);
// 		batch->establish(BatchAspect::LIVE_VERTICES, BatchAspect::LIVE_INSTANCES, BatchAspect::LIVE_TEXTURE); // TODO use sheduler worker with own commandBuffer as worker for this task
// 		batch->free(BatchAspect::STAGING_VERTICES, BatchAspect::STAGING_TEXTURE); // TODO free working versices & texture also
// 		batchesPtr[it.first] = it.second.get();
// 		lava.addBatch(move(batch));
// 	}

// 	auto time = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count();
// 	printf("Established %lld batches (%lld Mb textures) in %.3fs (%.2f Gb/s)\n", batches.size(), texturesBytes / (1 << 20), time, texturesBytes / time / (1 << 30));
// }



// void Batcher::establish(Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
// 	this->lava = &lava;

// 	auto start = chrono::high_resolution_clock::now();

// 	VkCommandBuffer cb_u;

// 	auto id0 = team.gpuTask([&rocks, &cb_u, this](VkCommandBuffer cb){
// 		cb_u = rocks.beginSingleTimeCommands();
// 	});

// 	set<shared_ptr<Task>> deps;
// 	for (auto& it : batches) {
// 		auto& key = it.first;
// 		auto& batch = it.second;

// 		auto id1 = team.task(ST_CPU, [&batch, &mountain, &rocks, &crater, &lava, this]{
// 			batch->setVulkanEntities(mountain, rocks, crater);
// 			batch->establish(BatchAspect::STAGING_VERTICES, BatchAspect::STAGING_INSTANCES, BatchAspect::STAGING_TEXTURE);
// 		});

// 		auto id2 = team.gpuTask([&key, &batch, &mountain, &rocks, &crater, &lava, this](VkCommandBuffer cb){
// 			batch->establish(BatchAspect::LIVE_VERTICES, BatchAspect::LIVE_INSTANCES, BatchAspect::LIVE_TEXTURE);
// 		}, { id1 });
// 		deps.insert(id2);
// 	}

// 	team.gpuTask([&rocks, &cb_u, this](VkCommandBuffer cb){
// 		rocks.endSingleTimeCommands(cb_u);
// 	}, { id0 });

// 	auto id3 = team.gpuTask([&rocks, this](VkCommandBuffer cb){
// 	}, deps);

// 	auto id4 = team.gpuTask([&rocks, &lava, this](VkCommandBuffer cb){
// 		for (auto& it : batches) {
// 			auto& key = it.first;
// 			auto& batch = it.second;
// 			batchesPtr[key] = batch.get();
// 			lava.addBatch(move(batch));
// 		}
// 	}, { id3 });

// 	auto time = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count();
// 	printf("Established %lld batches (%lld Mb textures) in %.3fs (%.2f Gb/s)\n", batches.size(), texturesBytes / (1 << 20), time, texturesBytes / time / (1 << 30));
// }



void Batcher::establish(Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
	this->lava = &lava;

	auto start = chrono::high_resolution_clock::now();

	set<shared_ptr<Task>> deps;
	for (auto& it : batches) {
		auto& batch = it.second;
		batch->setVulkanEntities(mountain, rocks, crater, lava);

		deps.insert(team.task(ST_CPU, [&batch, this]{
			batch->establish(BatchAspect::STAGING_VERTICES, BatchAspect::STAGING_INSTANCES, BatchAspect::STAGING_TEXTURE);
		}));
	}

	auto id = team.gpuTask([this](VkCommandBuffer cb){
		for (auto& it : batches) {
			auto& batch = it.second;
			batch->establish(cb, BatchAspect::LIVE_VERTICES, BatchAspect::LIVE_INSTANCES, BatchAspect::LIVE_TEXTURE);
		}
	}, deps);

	team.gpuTask([&lava, this](VkCommandBuffer cb){
		for (auto& it : batches) {
			auto& key = it.first;
			auto& batch = it.second;
			batch->free(BatchAspect::STAGING_VERTICES, BatchAspect::STAGING_TEXTURE); // TODO free working versices & texture also
			batch->createDescriptorSet();
			batchesPtr[key] = batch.get();
			lava.addBatch(move(batch));
		}
	}, { id });

	auto time = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count();
	printf("Established %lld batches (%lld Mb textures) in %.3fs (%.2f Gb/s)\n", batches.size(), texturesBytes / (1 << 20), time, texturesBytes / time / (1 << 30));
}



size_t Batcher::addInstance(string name, Instance instance) {
	size_t result = 0;

	if (batchesPtr[name]->vacuum.size() == 0) {
		size_t oldSize = batchesPtr[name]->instances.size();
		size_t newSize = (oldSize == 0) ? 1 : oldSize * 2;

		batchesPtr[name]->vacuum.reserve(newSize);
		batchesPtr[name]->instances.reserve(newSize);
		batchesPtr[name]->instances.push_back(instance);

		for (size_t i = oldSize + 1; i < newSize; i++) {
			batchesPtr[name]->instances.push_back(VACUUM);
			batchesPtr[name]->vacuum.push_back(i);
		}

		resizedNames.insert(name);
		// printf("Batch '%s' marked for resize for %lld instances\n", name.data(), batchesPtr[name]->instances.size());
		result = oldSize;

	} else {
		size_t freeSlot = batchesPtr[name]->vacuum.back();
		batchesPtr[name]->vacuum.pop_back();
		batchesPtr[name]->instances[freeSlot] = instance;
		result = freeSlot;
	}

	touchedIndexes[name].push_back(result);

	return result;
}

void Batcher::removeInstance(string name, size_t index) {
	#ifdef use_validation
	char msg[100];
	sprintf(msg, "Batcher::removeInstance(%s, %lld) => No such index!\n", name.data(), index);
	(index < batchesPtr[name]->instances.size()) >> ash(msg);
	#endif

	batchesPtr[name]->instances[index] = VACUUM;
	batchesPtr[name]->vacuum.push_back(index);
	touchedIndexes[name].push_back(index);
}

void Batcher::updateInstance(string name, size_t index, Instance instance) {
	#ifdef use_validation
	char msg[100];
	sprintf(msg, "Batcher::updateInstance(%s, %lld) => No such index!\n", name.data(), index);
	(index < batchesPtr[name]->instances.size()) >> ash(msg);
	#endif

	batchesPtr[name]->instances[index] = instance;
	touchedIndexes[name].push_back(index);
}

void Batcher::update(double t, double dt) {
	for (auto& [name, value] : touchedIndexes) {
		if (resizedNames.find(name) != resizedNames.end() || value.size() * 100 > batchesPtr[name]->instanceCount) {
			batchesPtr[name]->refresh(BatchAspect::STAGING_INSTANCES, BatchAspect::LIVE_INSTANCES);
		} else if (value.size() > 0) {
			batchesPtr[name]->updateInstances(value);
		}
		value.clear();
	}

	resizedNames.clear();
}
