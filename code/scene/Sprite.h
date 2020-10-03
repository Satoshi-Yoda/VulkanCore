#ifndef SPRITE_H
#define SPRITE_H

#include <string>
#include <vector>

#include "../engine/Lava.h"
#include "../engine/Tectonic.h"

class Sprite {
public:
	Sprite(string name);
	~Sprite();

private:
	string name;

	void load();
	void establish(Lava &lava, Tectonic &tectonic);
};

#endif