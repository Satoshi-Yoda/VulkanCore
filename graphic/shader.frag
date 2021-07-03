#version 450
#extension GL_ARB_separate_shader_objects : enable

struct GraphicElement {
	float value;
};

layout(binding = 1) readonly buffer GraphicData {
	vec4 color;
	vec2 size;
	float radius;
	float aa;
	float line;
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

	//return 0.333f / pow(distance, 0.8f);

	float line  = data.line * 0.5f;
	float delta = data.aa;
	float onLine = 1.0f - smoothstep(line, line + delta, distance);
	return onLine;
}

float capsule_if(vec2 A, vec2 B) {
	vec2 o = fragTexCoord;
	float distance_A = length(A - o);
	float thr = length(A - B) + data.line + data.aa;
	if (distance_A > thr) {
		return 0.0f;
	}

	float distance = 0.0f;
	if (dot(B - A, o - A) < 0.0f) {
		distance = distance_A;
	}
	else if (dot(A - B, o - B) < 0.0f) {
		float distance_B = length(B - o);
		distance = distance_B;
	}
	else {
		float inverse_width = 1.0f / (B.x - A.x);

		float a_x = fragTexCoord.x - 0.5f;
		float part_a = (a_x - A.x) * inverse_width;
		vec2 a = vec2(a_x, A.y + part_a * (B.y - A.y));

		float b_x = fragTexCoord.x + 0.5f;
		float part_b = (b_x - A.x) * inverse_width;
		vec2 b = vec2(b_x, A.y + part_b * (B.y - A.y));

		vec2 ba = b - a;

		distance = abs(ba.y * o.x - ba.x * o.y + b.x * a.y - a.x * b.y) / length(ba);
	}

	float line  = data.line * 0.5f;
	float onLine = 1.0f - smoothstep(line, line + data.aa, distance);
	return onLine;
}

void main() {
	float total = 0.0f;
	//total += capsule(vec2(200, 200), data.size - vec2(200, 200));
	//total += capsule(vec2(400, 200), data.size - vec2(200, 400));

	// TODO optimize, cals only nearest data points
	for (int i = 0; i <= data.points.length() - 2; i++) {
		vec2 A = vec2(float(i    ) / (data.points.length() - 1.0f), data.points[i    ].value) * data.size;
		vec2 B = vec2(float(i + 1) / (data.points.length() - 1.0f), data.points[i + 1].value) * data.size;
		total = max(total, capsule_if(A, B));
	}

	vec4 result = mix(data.color, vec4(1.0f), clamp(total, 0.0f, 1.0f));

	vec2 texCoordRounded = round(fragTexCoord);
	vec2 halfSize = data.size * 0.5f;
	vec2 fromCorner = halfSize - abs(texCoordRounded - halfSize);
	vec2 cornerPoint = vec2(data.radius);
	float fromCornerPoint = length(cornerPoint - fromCorner);
	float halfStep    = data.aa * 0.5f;
	float quarterStep = data.aa * 0.25f;
	float alpha = smoothstep(data.radius + quarterStep - halfStep, data.radius + quarterStep + halfStep, fromCornerPoint);
	float inCorner = float(all(lessThan(fromCorner, cornerPoint + vec2(1.0f))));
	float outAlpha = (1.0f - inCorner * alpha) * data.color.a;

	outColor = vec4(result.rgb, outAlpha);
}
