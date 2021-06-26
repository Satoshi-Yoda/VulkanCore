#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(binding = 0) uniform UniformBufferObject {
	vec2 scale;
	vec2 shift;
} ubo;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4((inPosition + ubo.shift) * ubo.scale, 0.0, 1.0);
}
