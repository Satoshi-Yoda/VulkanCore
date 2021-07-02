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
	vec2 delta_x = vec2(1.0f, 0.0f);

	vec2 coord1 = (fragTexCoord + delta_x) / data.size;
	float arrayCoord1 = coord1.x * (data.points.length() - 1);
	int index1 = int(floor(arrayCoord1));
	float part1 = fract(arrayCoord1);
	float value1 = data.points[index1 + 1].value * part1 + data.points[index1].value * (1.0f - part1);

	vec2 coord2 = (fragTexCoord - delta_x) / data.size;
	float arrayCoord2 = coord2.x * (data.points.length() - 1);
	int index2 = int(floor(arrayCoord2));
	float part2 = fract(arrayCoord2);
	float value2 = data.points[index2 + 1].value * part2 + data.points[index2].value * (1.0f - part2);

	//float tan = (data.points[index + 1].value - data.points[index].value) / (data.points.length());

	//float mult = (1.0f + 1000 * abs(tan));
	float line  = 1.0f / data.size.y;
	float delta = 1.0f / data.size.y;
	//float onLine = step(line + delta, min(abs(coord1.y - value1), abs(coord2.y - value2)));
	float onLine1 = step(line + delta, coord1.y - value1);
	float onLine2 = step(line + delta, coord2.y - value2);
	float onLine = 1.0f - abs(onLine1 - onLine2);
	//float onLine = smoothstep(line, line + delta, min(abs(coord1.y - value1), abs(coord2.y - value2)));

	vec4 result = mix(vec4(1.0f), data.color, onLine);

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
