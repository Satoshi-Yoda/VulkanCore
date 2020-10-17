#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform UniformBufferObject {
	vec2 scale;
	vec2 shift;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

out gl_PerVertex {
	vec4 gl_Position;
};

// layout(location = 0) out vec3 fragColor;
layout(location = 0) out vec2 fragTexCoord;

void main() {
	gl_Position = vec4((inPosition + ubo.shift) * ubo.scale, 0.0, 1.0);
	fragTexCoord = inTexCoord;
}
