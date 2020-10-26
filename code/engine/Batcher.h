#ifndef BATCHER_H
#define BATCHER_H

#include <string>
// #include <vector>

// #include "../core/Lava.h"
// #include "../core/Tectonic.h"

using std::string;

class Batcher {
public:
	Batcher();
	~Batcher();

private:
	void loadFolder(string folder);
};

#endif