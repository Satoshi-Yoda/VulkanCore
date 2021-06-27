#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) readonly buffer RectangleData {
	vec4 color;
	vec2 size;
	float radius;
	float step;
} data;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	vec2 halfSize = data.size / 2.0f;
	//vec2 fromCorner = halfSize - abs(fragTexCoord - halfSize);
	vec2 fromCorner = fragTexCoord;
	vec2 cornerPoint = vec2(data.radius, data.radius);
	float fromCornerPoint = length(cornerPoint - fromCorner);
	float halfStep = data.step / 2.0f;
	float alpha = smoothstep(data.radius - halfStep, data.radius + halfStep, fromCornerPoint);
	float inCorner = float(all(lessThan(fromCorner, cornerPoint)));
	outColor = vec4(data.color.rgb, 1.0f - inCorner * alpha);
}
