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
	vec2 texCoordRounded = round(fragTexCoord);
	vec2 halfSize = data.size * 0.5f;
	vec2 fromCorner = halfSize - abs(texCoordRounded - halfSize);
	vec2 cornerPoint = vec2(data.radius);
	float fromCornerPoint = length(cornerPoint - fromCorner);
	float halfStep    = data.step * 0.5f;
	float quarterStep = data.step * 0.25f;
	float alpha = smoothstep(data.radius + quarterStep - halfStep, data.radius + quarterStep + halfStep, fromCornerPoint);
	float inCorner = float(all(lessThan(fromCorner, cornerPoint + vec2(1.0f))));
	outColor = vec4(data.color.rgb, 1.0f - inCorner * alpha);
}
