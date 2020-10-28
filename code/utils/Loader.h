#ifndef LOADER_H
#define LOADER_H

#include <string>

using std::string;

void loadTexture(string filename, void* &data, int *width, int *height, bool silent = true);
void freeTexture(void* &data);

#endif
