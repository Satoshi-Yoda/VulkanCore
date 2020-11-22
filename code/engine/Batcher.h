#ifndef BATCHER_H
#define BATCHER_H

#include <string>
#include <unordered_map>
#include <map>
#include <vector>

#include "../core/Lava.h"

using std::string;
using std::unordered_map;
using std::map;

class Batcher {
public:
	Batcher();
	~Batcher();

	void loadFolder(string folder);
	void loadFolderNth(string folder, uint32_t workers = 1);
	void establish(Lava& lava);
	void addInstance(string name, Instance instance);
	void updateInstance(string name, size_t index, Instance instance);
	void update(double t, double dt);

private:
	Lava* lava;

	// TODO remove
	map<string, int> width, height;
	map<string, void*> pixels;
	map<string, vector<Vertex>> vertices;
	map<string, vector<Instance>> instances;
	map<string, size_t> indexes;

	map<string, BatchCreateData> batches;
	vector<string> namesForUpdate;

	size_t texturesBytes;

	void initQuad(string name, uint32_t w, uint32_t h);
	void addSampleInstance(string name);
};

#endif