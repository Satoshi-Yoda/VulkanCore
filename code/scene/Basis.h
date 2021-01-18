#pragma once

#include <string>
#include <vector>

#include "../core/Lava.h"
#include "../core/Tectonic.h"

// - contatins all data about where child objects will be drawn
// - can have parent basis
// - can have viewport/scissor rectangle(s)
// - can be used to generate baked textures
// - can be curved

class Basis {
public:
	Basis(string name);
	~Basis();

private:
	string name;

	void load();
	void establish(Lava &lava, Tectonic &tectonic);
};
