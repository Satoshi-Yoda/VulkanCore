VULKAN_HOME     = C:/_soft/VulkanSDK_1.2.148.1
GLFW_HOME       = O:/_libs/glfw-3.3.2.bin.WIN64
GLM_HOME        = O:/_libs/glm-edited
# STB_HOME        = O:/_libs/stb-master
STB_HOME        = O:/_repo/stb
VMA_HOME        = O:/_libs/VulkanMemoryAllocator-master/src
MAGIC_ENUM_HOME = O:/_libs/magic_enum-master/include
CATCH_HOME      = O:/_libs/Catch2-amalgamated

VULKAN_INCLUDE     = ${VULKAN_HOME}/Include
GLFW_INCLUDE       = ${GLFW_HOME}/include
GLM_INCLUDE        = ${GLM_HOME}
STB_INCLUDE        = ${STB_HOME}
VMA_INCLUDE        = ${VMA_HOME}
MAGIC_ENUM_INCLUDE = ${MAGIC_ENUM_HOME}
CATCH_INCLUDE      = ${CATCH_HOME}

VULKAN_LIB = ${VULKAN_HOME}/Lib
GLFW_LIB   = ${GLFW_HOME}/lib-mingw-w64

ifdef tests_here
	INCLUDE = -I${VULKAN_INCLUDE} -I${GLFW_INCLUDE} -I${GLM_INCLUDE} -I${STB_INCLUDE} -I${VMA_INCLUDE} -I${MAGIC_ENUM_INCLUDE} -I${CATCH_INCLUDE}
	LINK = ${VULKAN_LIB}/vulkan-1.lib ${GLFW_LIB}/glfw3.dll ${CATCH_HOME}/catch_amalgamated.o
	COMPILE = -std=c++20 -O3 -msse2 ${gcc_params} -Dtests_here
else
	INCLUDE = -I${VULKAN_INCLUDE} -I${GLFW_INCLUDE} -I${GLM_INCLUDE} -I${STB_INCLUDE} -I${VMA_INCLUDE} -I${MAGIC_ENUM_INCLUDE}
	LINK = ${VULKAN_LIB}/vulkan-1.lib ${GLFW_LIB}/glfw3.dll
	COMPILE = -std=c++20 -O3 -msse2 ${gcc_params}
endif

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
