#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) readonly buffer RectangleData {
	vec4 color;
} data;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = data.color;
}
