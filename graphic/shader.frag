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
	vec2 delta_x = vec2(0.5f, 0.0f);

	vec2 coord1 = (fragTexCoord + delta_x) / data.size;
	float arrayCoord1 = coord1.x * (data.points.length() - 1);
	int index1 = int(floor(arrayCoord1));
	float part1 = fract(arrayCoord1);
	float value1 = data.points[index1 + 1].value * part1 + data.points[index1].value * (1.0f - part1);
	vec2 A = vec2(coord1.x, value1) * data.size;

	vec2 coord2 = (fragTexCoord - delta_x) / data.size;
	float arrayCoord2 = coord2.x * (data.points.length() - 1);
	int index2 = int(floor(arrayCoord2));
	float part2 = fract(arrayCoord2);
	float value2 = data.points[index2 + 1].value * part2 + data.points[index2].value * (1.0f - part2);
	vec2 B = vec2(coord2.x, value2) * data.size;

	vec2 BA = B - A;
	vec2 O = fragTexCoord;
	float distance = abs(BA.y * O.x - BA.x * O.y + B.x * A.y - A.x * B.y) / length(BA);

	float line  = 0.0f;
	float delta = 1.0f;
	float onLine = smoothstep(line, line + delta, distance);
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
