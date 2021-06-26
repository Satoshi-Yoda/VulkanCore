#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(0.5f, 0.5f, 0.5f, 0.5f);
}
