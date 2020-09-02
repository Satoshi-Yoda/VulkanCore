VULKAN_HOME = D:/Soft/VulkanSDK/1.2.148.1
GLFW_HOME   = D:/Soft/glfw-3.3.2.bin.WIN64
GLM_HOME    = D:/Soft/glm

VULKAN_INCLUDE = ${VULKAN_HOME}/Include
GLFW_INCLUDE   = ${GLFW_HOME}/include
GLM_INCLUDE    = ${GLM_HOME}

VULKAN_LIB = ${VULKAN_HOME}/Lib
GLFW_LIB   = ${GLFW_HOME}/lib-mingw-w64

COMPILE = -std=c++17 -O3
INCLUDE = -I${VULKAN_INCLUDE} -I${GLFW_INCLUDE} -I${GLM_INCLUDE}
LINK = ${VULKAN_LIB}/vulkan-1.lib ${GLFW_LIB}/glfw3.dll

main.exe : main.o vectors.o shaders/shader.vert.spv shaders/shader.frag.spv
	g++ main.o vectors.o ${LINK} -o main.exe

main.o : main.cpp
	g++ ${COMPILE} ${INCLUDE} -c main.cpp

vectors.o : vectors.cpp
	g++ ${COMPILE} ${INCLUDE} -c vectors.cpp

shaders/shader.vert.spv : shaders/shader.vert
	glslc shaders/shader.vert -o shaders/shader.vert.spv

shaders/shader.frag.spv : shaders/shader.frag
	glslc shaders/shader.frag -o shaders/shader.frag.spv
