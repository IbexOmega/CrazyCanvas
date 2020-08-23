float normalDistanceCos(vec3 n1, vec3 n2, float power)
{
	//return pow(max(0.0, dot(n1, n2)), 128.0);
	//return pow( saturate(dot(n1,n2)), power);
	return 1.0f;
}

float normalDistanceTan(vec3 a, vec3 b)
{
	const float d = max(1e-8f, dot(a, b));
	return sqrt(max(0.0f, 1.0f - d * d)) / d;
}

vec2 computeWeight(
	float depthCenter, float depthP, float phiDepth,
	vec3 normalCenter, vec3 normalP, float normPower, 
	float luminanceDirectCenter, float luminanceDirectP, float phiDirect,
	float luminanceIndirectCenter, float luminanceIndirectP, float phiIndirect)
{
	const float wNormal    = normalDistanceCos(normalCenter, normalP, normPower);
	const float wZ         = (phiDepth == 0) ? 0.0f : abs(depthCenter - depthP) / phiDepth;
	const float wLdirect   = abs(luminanceDirectCenter - luminanceDirectP) / phiDirect;
	const float wLindirect = abs(luminanceIndirectCenter - luminanceIndirectP) / phiIndirect;

	const float wDirect   = exp(0.0f - max(wLdirect, 0.0f)   - max(wZ, 0.0f)) * wNormal;
	const float wIndirect = exp(0.0f - max(wLindirect, 0.0f) - max(wZ, 0.0f)) * wNormal;

	return vec2(wDirect, wIndirect);
}