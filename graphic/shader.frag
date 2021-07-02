#version 450
#extension GL_ARB_separate_shader_objects : enable

struct GraphicElement {
	float value;
};

layout(binding = 1) readonly buffer GraphicData {
	vec4 color;
	vec2 size;
	float radius;
	float step;
	GraphicElement points[];
} data;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	vec2 coord = fragTexCoord / data.size;
	float arrayCoord = coord.x * (data.points.length() - 1);
	int index = int(floor(arrayCoord));
	float part = fract(arrayCoord);
	float value = data.points[index + 1].value * part + data.points[index].value * (1.0f - part);
	vec4 result = (coord.y > value) ? data.color : (data.color * 0.8f);

	vec2 texCoordRounded = round(fragTexCoord);
	vec2 halfSize = data.size * 0.5f;
	vec2 fromCorner = halfSize - abs(texCoordRounded - halfSize);
	vec2 cornerPoint = vec2(data.radius);
	float fromCornerPoint = length(cornerPoint - fromCorner);
	float halfStep    = data.step * 0.5f;
	float quarterStep = data.step * 0.25f;
	float alpha = smoothstep(data.radius + quarterStep - halfStep, data.radius + quarterStep + halfStep, fromCornerPoint);
	float inCorner = float(all(lessThan(fromCorner, cornerPoint + vec2(1.0f))));
	float outAlpha = (1.0f - inCorner * alpha) * data.color.a;

	outColor = vec4(result.rgb, outAlpha);
}
