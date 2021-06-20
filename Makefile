VULKAN_HOME     = C:/_soft/VulkanSDK_1.2.148.1
GLFW_HOME       = O:/_libs/glfw-3.3.2.bin.WIN64
GLM_HOME        = O:/_libs/glm
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
O_FILES = $(subst code-,temp/,$(subst /,-,$(patsubst %.cpp,%.o,${CPP_FILES})))

GLSL_FILES = $(wildcard */*/*.vert) $(wildcard */*/*.frag)
SPV_FILES = $(addprefix build/shaders/,$(addsuffix .spv,$(subst .,-,$(subst -shader,,$(subst code-,,$(subst /,-,${GLSL_FILES}))))))

build/main.exe : ${O_FILES} ${SPV_FILES}
	g++ ${COMPILE} ${O_FILES} ${LINK} -o build/main.exe

define CPP_O_RECIPE
$(1)
	g++ $${COMPILE} $${INCLUDE} -c $$^ -o $$@
endef
$(foreach file,$(join $(addsuffix :,$(O_FILES)),$(CPP_FILES)),$(eval $(call CPP_O_RECIPE,$(file))))

define GLSL_SPV_RECIPE
$(1)
	glslc $$^ -o $$@
endef
$(foreach file,$(join $(addsuffix :,$(SPV_FILES)),$(GLSL_FILES)),$(eval $(call GLSL_SPV_RECIPE,$(file))))
