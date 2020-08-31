VULKAN_INCLUDE = -I"D:/Soft/VulkanSDK/1.2.148.1/Include"
GLFW_INCLUDE = -I"D:/Soft/glfw-3.3.2.bin.WIN64/include"
GLM_INCLUDE = -I"D:/Soft/glm"

VULKAN_LIB = -L"D:/Soft/VulkanSDK/1.2.148.1/Lib"
GLFW_LIB = -L"D:/Soft/glfw-3.3.2.bin.WIN64/lib-mingw-w64"

# COMPILER_FLAGS = -std=c++17 -O3 -L${MKL_LIB_DIR} -I${MKL_INCLUDE_DIR}




COMPILER_FLAGS = -std=c++17 -O3

main.exe : main.cpp vectors.o
	g++ ${COMPILER_FLAGS} ${VULKAN_INCLUDE} ${GLFW_INCLUDE} ${GLM_INCLUDE} ${VULKAN_LIB} ${GLFW_LIB} main.cpp vectors.o glfw3.dll vulkan-1.lib -o main.exe

vectors.o : vectors.cpp
	g++ ${COMPILER_FLAGS} -c vectors.cpp

# shaders/shader.vert.spv : shaders/shader.vert
	# glslc shaders/shader.vert -o shaders/shader.vert.spv

# shaders/shader.frag.spv : shaders/shader.frag
	# glslc shaders/shader.frag -o shaders/shader.frag.spv

#vulkan-1.lib libglfw3.a
