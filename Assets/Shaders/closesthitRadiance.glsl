#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "helpers.glsl"

struct RayPayload
{
	vec3 Radiance;
	uint Recursion;
};

struct ShadowRayPayload
{
	float Occlusion;
};

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

struct MaterialParameters
{
	vec4 Albedo;
	float Metallic;
	float Roughness;
	float AO;
	float Padding;
};

struct PointLight
{
	vec4 Color;
	vec4 Position;
};

layout(location = 0) rayPayloadInNV RayPayload rayPayload;
layout(location = 1) rayPayloadNV ShadowRayPayload shadowRayPayload;

hitAttributeNV vec3 attribs;

// Max. number of recursion is passed via a specialization constant
layout (constant_id = 0) const int MAX_RECURSION = 0;
layout (constant_id = 1) const int MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES = 16;
layout (constant_id = 2) const int MAX_POINT_LIGHTS = 4;

layout(binding = 2, set = 0) uniform accelerationStructureNV u_TopLevelAS;
layout(binding = 6, set = 0) buffer Vertices { Vertex v[]; } u_SceneVertices;
layout(binding = 7, set = 0) buffer Indices { uint i[]; } u_SceneIndices;
layout(binding = 8, set = 0) buffer MeshIndices { uint mi[]; } u_MeshIndices;
layout(binding = 9 , set = 0) uniform sampler2D u_SceneAlbedoMaps[MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES];
layout(binding = 10, set = 0) uniform sampler2D u_SceneNormalMaps[MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES];
layout(binding = 11, set = 0) uniform sampler2D u_SceneAOMaps[MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES];
layout(binding = 12, set = 0) uniform sampler2D u_SceneMetallicMaps[MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES];
layout(binding = 13, set = 0) uniform sampler2D u_SceneRougnessMaps[MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES];
layout(binding = 14, set = 0) buffer CombinedMaterialParameters 
{ 
	MaterialParameters mp[]; 
} u_MaterialParameters;
layout (binding = 16) uniform LightBuffer
{
	PointLight lights[MAX_POINT_LIGHTS];
} u_Lights;
layout(binding = 18, set = 0) uniform sampler2D u_BrdfLUT;
layout(binding = 20, set = 0) uniform sampler2D u_BlueNoiseLUT;

layout (push_constant) uniform PushConstants
{
	float Counter;
	float MaxTemporalFrames;
	float MinTemporalWeight;
	float ReflectionRayBias;
	float ShadowRayBias;
} u_PushConstants;

vec3 myRefract(vec3 I, vec3 N, float ior)
{
    float cosi = clamp(-1.0f, 1.0f, dot(I, N));
    float etai = 1;
	float etat = ior;
    vec3 n = N;
    if (cosi < 0)
	{
		cosi = -cosi;
	}
	else
	{
		float temp = etai;
		etai = etat;
		etat = temp;
		n = -N;
	}
    float eta = etai / etat;
    float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
    return k < 0.0f ? vec3(0.0f) : eta * I + (eta * cosi - sqrt(k)) * n;
}

void calculateTriangleData(out uint materialIndex, out vec2 texCoords, out vec3 normal)
{
	materialIndex = 	u_MeshIndices.mi[3 * gl_InstanceCustomIndexNV + 2];

	uint meshVertexOffset = u_MeshIndices.mi[3 * gl_InstanceCustomIndexNV];
	uint meshIndexOffset = 	u_MeshIndices.mi[3 * gl_InstanceCustomIndexNV + 1];
	ivec3 index = ivec3(u_SceneIndices.i[meshIndexOffset + 3 * gl_PrimitiveID], u_SceneIndices.i[meshIndexOffset + 3 * gl_PrimitiveID + 1], u_SceneIndices.i[meshIndexOffset + 3 * gl_PrimitiveID + 2]);

	Vertex v0 = u_SceneVertices.v[meshVertexOffset + index.x];
	Vertex v1 = u_SceneVertices.v[meshVertexOffset + index.y];
	Vertex v2 = u_SceneVertices.v[meshVertexOffset + index.z];

	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	texCoords = (v0.TexCoord.xy * barycentricCoords.x + v1.TexCoord.xy * barycentricCoords.y + v2.TexCoord.xy * barycentricCoords.z);

	mat4 transform;
	transform[0] = vec4(gl_ObjectToWorldNV[0], 0.0f);
	transform[1] = vec4(gl_ObjectToWorldNV[1], 0.0f);
	transform[2] = vec4(gl_ObjectToWorldNV[2], 0.0f);
	transform[3] = vec4(gl_ObjectToWorldNV[3], 1.0f);

	vec3 T = normalize(v0.Tangent.xyz * barycentricCoords.x + v1.Tangent.xyz * barycentricCoords.y + v2.Tangent.xyz * barycentricCoords.z);
	vec3 N  = normalize(v0.Normal.xyz * barycentricCoords.x + v1.Normal.xyz * barycentricCoords.y + v2.Normal.xyz * barycentricCoords.z);

	T = normalize(vec3(transform * vec4(T, 0.0)));
	N = normalize(vec3(transform * vec4(N, 0.0)));
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);

	normal = texture(u_SceneNormalMaps[materialIndex], texCoords).xyz;
	normal = normalize(normal * 2.0f - 1.0f);
	normal = TBN * normal;
}

void main()
{
	uint recursionNumber = rayPayload.Recursion;

	uint materialIndex = 0;
	vec2 texCoords = vec2(0.0f);
	vec3 normal = vec3(0.0f);
	calculateTriangleData(materialIndex, texCoords, normal);

	//Define Constants
	vec3 hitPos = gl_WorldRayOriginNV + normalize(gl_WorldRayDirectionNV) * gl_HitTNV;

	//Define new Rays Parameters
	uint rayFlags = gl_RayFlagsOpaqueNV;
	uint cullMask = 0xff;
	float tmin = 0.001f;
	float tmax = 10000.0f;

	//Sample rest of textures
	vec3 sampledAlbedo = texture(u_SceneAlbedoMaps[materialIndex], texCoords).rgb;
	float sampledMetallic = texture(u_SceneMetallicMaps[materialIndex], texCoords).r;
	float sampledRoughness = texture(u_SceneRougnessMaps[materialIndex], texCoords).r;
	float sampledAO = texture(u_SceneAOMaps[materialIndex], texCoords).r;

	//Combine Samples with Material Parameters
	MaterialParameters mp = u_MaterialParameters.mp[materialIndex];
	vec3 albedo = mp.Albedo.rgb * sampledAlbedo;
	float metallic = mp.Metallic * sampledMetallic;
	float roughness = max(mp.Roughness * sampledRoughness, 0.00001f);
	float ao = mp.AO * sampledAO;
	
	//Init BRDF values
	vec3 viewDir = -gl_WorldRayDirectionNV;

	vec3 f0 = vec3(0.04f);
	f0 = mix(f0, albedo, metallic);

	float metallicFactor = 1.0f - metallic;
	
	float NdotV = max(dot(viewDir, normal), 0.0f);

	//Irradiance from surroundings
	vec3 diffuse 	= albedo;
	vec3 f 			= FresnelRoughness(f0, NdotV, roughness);
	vec3 kDiffuse 	= (vec3(1.0f) - f) * metallicFactor;

	vec3 L0 = vec3(0.0f);
	vec3 specular = vec3(0.0f);

	if (recursionNumber < MAX_RECURSION)
	{
		vec3 shadowRaysOrigin = hitPos + normal * u_PushConstants.ShadowRayBias;

		for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			vec3 lightPosition 	= u_Lights.lights[i].Position.xyz;
			vec3 lightColor 	= u_Lights.lights[i].Color.rgb;

			vec3 lightVector 	= (lightPosition - hitPos);
			vec3 lightDir 		= normalize(lightVector);

			traceNV(u_TopLevelAS, rayFlags, cullMask, 1, 0, 1, shadowRaysOrigin, tmin, lightDir, tmax, 1);

			if (shadowRayPayload.Occlusion < 0.1f)
			{
				float lightDistance	= length(lightVector);
				float attenuation 	= 1.0f / (lightDistance * lightDistance);

				vec3 halfVector = normalize(viewDir + lightDir);

				vec3 radiance = lightColor * attenuation;

				float HdotV = max(dot(halfVector, viewDir), 0.0f);
				float NdotL = max(dot(normal, lightDir), 0.0f);

				vec3 f 		= Fresnel(f0, HdotV);
				float ndf 	= Distribution(normal, halfVector, roughness);
				float g 	= GeometryOpt(NdotV, NdotL, roughness);

				float denom 	= 4.0f * NdotV * NdotL + 0.0001f;
				vec3 specular   = (ndf * g * f) / denom;

				//Take 1.0f minus the incoming radiance to get the diffuse (Energy conservation)
				vec3 diffuse = (vec3(1.0f) - f) * metallicFactor;

				L0 += ((diffuse * (albedo / PI)) + specular) * radiance * NdotL;
			}
		}		

		vec3 reflectionRaysOrigin = hitPos + normal * u_PushConstants.ReflectionRayBias;
		vec3 reflDir = reflect(gl_WorldRayDirectionNV, normal);
		//reflDir = ReflectanceDirection(hitPos + u_PushConstants.Counter, reflDir, roughness);

		rayPayload.Radiance = vec3(0.0f);
		rayPayload.Recursion = recursionNumber + 1;
		traceNV(u_TopLevelAS, rayFlags, cullMask, 0, 0, 0, reflectionRaysOrigin, tmin, reflDir, tmax, 0);

		vec2 envBRDF 	= texture(u_BrdfLUT, vec2(NdotV, roughness)).rg;
		specular		= rayPayload.Radiance * (f * envBRDF.x + envBRDF.y);		
	}
	else
	{
		for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			vec3 lightPosition 	= u_Lights.lights[i].Position.xyz;
			vec3 lightColor 	= u_Lights.lights[i].Color.rgb;

			vec3 lightVector 	= (lightPosition - hitPos);
			vec3 lightDir 		= normalize(lightVector);

			float lightDistance	= length(lightVector);
			float attenuation 	= 1.0f / max((lightDistance * lightDistance), 0.0001f);

			vec3 halfVector = normalize(viewDir + lightDir);

			vec3 radiance = lightColor * attenuation;

			float HdotV = max(dot(halfVector, viewDir), 0.0f);
			float NdotL = max(dot(normal, lightDir), 0.0f);

			vec3 f 		= Fresnel(f0, HdotV);
			float ndf 	= Distribution(normal, halfVector, roughness);
			float g 	= GeometryOpt(NdotV, NdotL, roughness);

			float denom 	= 4.0f * NdotV * NdotL + 0.0001f;
			vec3 specular   = (ndf * g * f) / denom;

			//Take 1.0f minus the incoming radiance to get the diffuse (Energy conservation)
			vec3 diffuse = (vec3(1.0f) - f) * metallicFactor;

			L0 += ((diffuse * (albedo / PI)) + specular) * radiance * NdotL;
		}	
	}

	vec3 ambient 	= ((kDiffuse * diffuse) + specular) * ao; //Approximate diffuse with albedo * vec3(0.03f)
	vec3 finalColor = ambient + L0;

	rayPayload.Radiance = finalColor;
}
