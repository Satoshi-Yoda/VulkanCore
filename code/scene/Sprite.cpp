#include "Sprite.h"

#include "../core/Lava.h"
#include "../core/Tectonic.h"
#include "../utils/Loader.h"

using namespace std;

Sprite::Sprite(string name) : name(name) {}

Sprite::~Sprite() {}

void Sprite::load() {
}

void Sprite::establish(Lava &lava, Tectonic &tectonic) {
}
