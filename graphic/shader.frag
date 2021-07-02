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

float capsule(vec2 A, vec2 B) {
	float inverse_width = 1.0f / (B.x - A.x);

	float a_x = fragTexCoord.x - 0.5f;
	float part_a = (a_x - A.x) * inverse_width;
	vec2 a = vec2(a_x, A.y + part_a * (B.y - A.y));

	float b_x = fragTexCoord.x + 0.5f;
	float part_b = (b_x - A.x) * inverse_width;
	vec2 b = vec2(b_x, A.y + part_b * (B.y - A.y));

	vec2 ba = b - a;
	vec2 o = fragTexCoord;
	float distance_line = abs(ba.y * o.x - ba.x * o.y + b.x * a.y - a.x * b.y) / length(ba);
	float distance_A = length(A - o);
	float distance_B = length(B - o);

	float dot_A = step(0.0f, dot(B - A, o - A));
	float dot_B = step(0.0f, dot(A - B, o - B));

	float distance = mix(distance_B, mix(distance_A, distance_line, dot_A), dot_B);

	float line  = 8.0f;
	float delta = 1.0f;
	float onLine = smoothstep(line, line + delta, distance);

	return distance / 100;
}

void main() {
	/*
	vec2 delta_x = vec2(0.5f, 0.0f);

	vec2 coord1 = (fragTexCoord + delta_x) / data.size;
	float arrayCoord1 = coord1.x * (data.points.length() - 1);
	int index1 = int(floor(arrayCoord1));
	float part1 = arrayCoord1 - index1;
	float value1 = data.points[index1 + 1].value * part1 + data.points[index1].value * (1.0f - part1);
	vec2 a = vec2(coord1.x, value1) * data.size;
	vec2 A = vec2(index1 / (data.points.length() - 1.0f), data.points[index1].value) * data.size;

	vec2 coord2 = (fragTexCoord - delta_x) / data.size;
	float arrayCoord2 = coord2.x * (data.points.length() - 1);
	int index2 = index1;
	float part2 = arrayCoord2 - index2;
	float value2 = data.points[index2 + 1].value * part2 + data.points[index2].value * (1.0f - part2);
	vec2 b = vec2(coord2.x, value2) * data.size;
	vec2 B = vec2((index1 + 1) / (data.points.length() - 1.0f), data.points[index1 + 1].value) * data.size;

	vec2 ba = b - a;
	vec2 o = fragTexCoord;
	float distance_line = abs(ba.y * o.x - ba.x * o.y + b.x * a.y - a.x * b.y) / length(ba);
	float distance_A = length(A - o);
	float distance_B = length(B - o);

	float dot_A = step(0.0f, dot(B - A, o - A));
	float dot_B = step(0.0f, dot(A - B, o - B));

	float distance = mix(distance_B, mix(distance_A, distance_line, dot_A), dot_B);

	float line  = 8.0f;
	float delta = 1.0f;
	float onLine = smoothstep(line, line + delta, distance);
	vec4 result = mix(vec4(1.0f), data.color, onLine);
	*/

	float caps = capsule(vec2(200, 200), data.size - vec2(200, 200));
	vec4 result = mix(vec4(1.0f), data.color, caps);

	//vec4 result = vec4(vec3(log(distance) / 5.0f), 1.0f);
	//vec4 result = vec4(vec3(0.5 + dot(B - A, o - A) / 10000), 1.0f);

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
