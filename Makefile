VULKAN_HOME = D:/Soft/VulkanSDK/1.2.148.1
GLFW_HOME   = D:/Soft/glfw-3.3.2.bin.WIN64
GLM_HOME    = D:/Soft/glm

VULKAN_INCLUDE = -I"${VULKAN_HOME}/Include"
GLFW_INCLUDE   = -I"${GLFW_HOME}/include"
GLM_INCLUDE    = -I"${GLM_HOME}"

VULKAN_LIB = -L"${VULKAN_HOME}/Lib"
GLFW_LIB   = -L"${GLFW_HOME}/lib-mingw-w64"

COMPILER_FLAGS = -std=c++17 -O3
INCLUDE = ${VULKAN_INCLUDE} ${GLFW_INCLUDE} ${GLM_INCLUDE} ${VULKAN_LIB} ${GLFW_LIB}
LINK = vulkan-1.lib glfw3.dll

main.exe : main.o vectors.o shaders/shader.vert.spv shaders/shader.frag.spv
	g++ main.o vectors.o ${LINK} -o main.exe

main.o : main.cpp
	g++ ${COMPILER_FLAGS} ${INCLUDE} -c main.cpp

vectors.o : vectors.cpp
	g++ ${COMPILER_FLAGS} ${INCLUDE} -c vectors.cpp

shaders/shader.vert.spv : shaders/shader.vert
	glslc shaders/shader.vert -o shaders/shader.vert.spv

shaders/shader.frag.spv : shaders/shader.frag
	glslc shaders/shader.frag -o shaders/shader.frag.spv
