#version 450
#extension GL_ARB_separate_shader_objects : enable

struct GraphicElement {
	float value;
};

layout(binding = 1) readonly buffer GraphicData {
	vec4 bgColor;
	vec4 lineColor;
	vec2 size;
	float radius;
	float aa;
	float line;
	GraphicElement points[];
} data;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

float capsule(vec2 A, vec2 B) {
	if (isnan(A.y) || isnan(B.y)) {
		return 0.0f;
	}

	vec2 o = vec2(fragTexCoord.x, data.size.y - fragTexCoord.y);

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

		float a_x = o.x - 0.5f;
		float part_a = (a_x - A.x) * inverse_width;
		vec2 a = vec2(a_x, A.y + part_a * (B.y - A.y));

		float b_x = o.x + 0.5f;
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
	vec2 o = vec2(fragTexCoord.x, data.size.y - fragTexCoord.y);

	float delta = 0.5f + data.line * 0.5f + data.aa;
	float min_x = o.x - delta;
	float max_x = o.x + delta;
	int first_i = 1;
	int last_i = data.points.length() - 1;
	int min_i = max(first_i + int(floor((last_i - first_i) * min_x / data.size.x)), first_i);
	int max_i = min(first_i + int( ceil((last_i - first_i) * max_x / data.size.x)), last_i);
	float scale = 1.0 / float(last_i - first_i);

	float total = 0.0f;
	for (int i = min_i; i < max_i; i++) {
		float vp = data.points[i - 1].value;
		float v0 = data.points[i    ].value;
		float v1 = data.points[i + 1].value;
		float v2 = data.points[i + 2].value;

		vec2 A;
		vec2 B;
		bool here = false;
		float s = 0.3;
		if (isnan(v0) && !isnan(v1) && isnan(v2)) {
			A = vec2((float(i + 1.0 - s) - first_i) * scale, v1) * data.size;
			B = vec2((float(i + 1.0    ) - first_i) * scale, v1) * data.size;
			here = true;
		} else if (isnan(vp) && !isnan(v0) && isnan(v1)) {
			A = vec2((float(i + 0.0) - first_i) * scale, v0) * data.size;
			B = vec2((float(i + s  ) - first_i) * scale, v0) * data.size;
			here = true;
		} else if (!isnan(v0) && !isnan(v1)) {
			A = vec2((float(i    ) - first_i) * scale, v0) * data.size;
			B = vec2((float(i + 1) - first_i) * scale, v1) * data.size;
			here = true;
		}
		if (here) {
			total = max(total, capsule(A, B));
		}
	}

	// vec4 result = mix(data.bgColor, data.lineColor, clamp(total, 0.0f, 1.0f));
	vec4 result = mix(data.bgColor, vec4(0.0, 0.0, 1.0, 1.0), clamp(total, 0.0f, 1.0f));

	vec2 texCoordRounded = round(o);
	vec2 halfSize = data.size * 0.5f;
	vec2 fromCorner = halfSize - abs(texCoordRounded - halfSize);
	vec2 cornerPoint = vec2(data.radius);
	float fromCornerPoint = length(cornerPoint - fromCorner);
	float halfStep    = data.aa * 0.5f;
	float quarterStep = data.aa * 0.25f;
	float alpha = smoothstep(data.radius + quarterStep - halfStep, data.radius + quarterStep + halfStep, fromCornerPoint);
	float inCorner = float(all(lessThan(fromCorner, cornerPoint + vec2(1.0f))));
	float outAlpha = (1.0f - inCorner * alpha) * data.bgColor.a;

	result.a *= (1.0f - inCorner * alpha);
	outColor = result;
}
