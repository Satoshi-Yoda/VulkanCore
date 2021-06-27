#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) readonly buffer RectangleData {
	vec4 color;
	vec2 size;
	float radius;
} data;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	vec2 halfSize = data.size / 2.0f;
	vec2 fromCorner = halfSize - abs(fragTexCoord - halfSize);
	outColor = vec4(data.color.rgb, length(fromCorner) / data.radius);
}
