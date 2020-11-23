#ifndef BATCHER_H
#define BATCHER_H

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "../core/Lava.h"

using std::map;
using std::set;
using std::string;
using std::unordered_map;

class Batcher {
public:
	Batcher();
	~Batcher();

	void loadFolder(string folder);
	void loadFolderNth(string folder, uint32_t workers = 1);
	void establish(Lava& lava);
	size_t addInstance(string name, Instance instance);
	void removeInstance(string name, size_t index);
	void updateInstance(string name, size_t index, Instance instance);
	void update(double t, double dt);

private:
	Lava* lava;

	const Instance VACUUM { { 1e16f, 1e16f } };

	map<string, BatchCreateData> batches;
	map<string, size_t> indexes;
	set<string> namesForUpdate;

	size_t texturesBytes;

	vector<Vertex> initQuad(uint32_t w, uint32_t h);
	void addSampleInstance(string name);
};

#endif