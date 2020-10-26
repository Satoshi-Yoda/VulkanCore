VULKAN_HOME  = C:/_soft/VulkanSDK_1.2.148.1
GLFW_HOME    = O:/_libs/glfw-3.3.2.bin.WIN64
GLM_HOME     = O:/_libs/glm
STB_HOME     = O:/_libs/stb-master

VULKAN_INCLUDE  = ${VULKAN_HOME}/Include
GLFW_INCLUDE    = ${GLFW_HOME}/include
GLM_INCLUDE     = ${GLM_HOME}
STB_INCLUDE     = ${STB_HOME}

VULKAN_LIB = ${VULKAN_HOME}/Lib
GLFW_LIB   = ${GLFW_HOME}/lib-mingw-w64

COMPILE = -std=c++17 -O3 -msse2
INCLUDE = -I${VULKAN_INCLUDE} -I${GLFW_INCLUDE} -I${GLM_INCLUDE} -I${STB_INCLUDE}
LINK = ${VULKAN_LIB}/vulkan-1.lib ${GLFW_LIB}/glfw3.dll

CPP_FILES = $(wildcard */*.cpp) $(wildcard */*/*.cpp)
O_REQUIREMENTS = $(patsubst %.cpp,%.o,${CPP_FILES})
# O_FILES = $(subst code-,temp/,$(subst /,-,${O_REQUIREMENTS}))

CODE = code
BUILD = build

# all:;echo $(O_FILES)

${BUILD}/main.exe : ${O_REQUIREMENTS} ${BUILD}/shaders/shader.vert.spv ${BUILD}/shaders/shader.frag.spv
	g++ ${COMPILE} ${O_REQUIREMENTS} ${LINK} -o ${BUILD}/main.exe

%.o : %.cpp
	g++ ${COMPILE} ${INCLUDE} -c $^ -o $@

# ${BUILD}/main.exe : ${O_FILES} ${BUILD}/shaders/shader.vert.spv ${BUILD}/shaders/shader.frag.spv
# 	g++ ${COMPILE} ${O_FILES} ${LINK} -o ${BUILD}/main.exe

# $(O_FILES) : %o : $(subst -,/,$(subst tempp/,code/,$(patsubst %.o,%.cpp,$@)))
# 	g++ ${COMPILE} ${INCLUDE} -c $(subst -,/,$(subst temp/,code/,$(patsubst %.o,%.cpp,$@))) -o $@

${BUILD}/shaders/shader.vert.spv : ${CODE}/shaders/shader.vert
	glslc ${CODE}/shaders/shader.vert -o ${BUILD}/shaders/shader.vert.spv

${BUILD}/shaders/shader.frag.spv : ${CODE}/shaders/shader.frag
	glslc ${CODE}/shaders/shader.frag -o ${BUILD}/shaders/shader.frag.spv
