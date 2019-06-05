
layout (std140) uniform CubeRadianceBlock
{
	int mode; // radiance, irradiance
	int size;
	int sampleCount;
} CubeRadianceParam;

vec3 get_world_normal()
{
	vec3 dir = (EvaluationParam.viewRot * vec4(vUV * 2.0 - 1.0, 1.0, 0.0)).xyz;
	return normalize(dir);
}
// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(vec2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt= dot(co.xy ,vec2(a,b));
	float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}

vec2 hammersley2d(uint i, uint N) 
{
	// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return vec2(float(i) /float(N), rdi);
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
vec3 importanceSample_GGX(vec2 Xi, float roughness, vec3 normal) 
{
	// Maps a 2D point to a hemisphere with spread based on roughness
	float alpha = roughness * roughness;
	float phi = 2.0 * PI * Xi.x + random(normal.xz) * 0.1;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	// Tangent space
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangentX = normalize(cross(up, normal));
	vec3 tangentY = normalize(cross(normal, tangentX));

	// Convert to world Space
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

// Normal Distribution function
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

vec3 prefilterEnvMap(vec3 R, float roughness)
{
	vec3 N = R;
	vec3 V = R;
	uint numSamples = uint(CubeRadianceParam.sampleCount);
	vec3 color = vec3(0.0);
	float totalWeight = float(numSamples);
	float envMapDim = float(textureSize(CubeSampler0, EvaluationParam.mipmapNumber).s);
	for(uint i = 0u; i < numSamples; i++) {
		vec2 Xi = hammersley2d(i, numSamples);
		vec3 H = importanceSample_GGX(Xi, roughness, N);
		vec3 L = 2.0 * dot(V, H) * H - V;
		
		color += textureLod(CubeSampler0, L, 0.).rgb;// * dotNL;
		/*
		float dotNL = clamp(dot(N, L), 0.0, 1.0);
		if(dotNL > 0.0) 
		{
			// Filtering based on https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/

			float dotNH = clamp(dot(N, H), 0.0, 1.0);
			float dotVH = clamp(dot(V, H), 0.0, 1.0);

			// Probability Distribution Function
			float pdf = D_GGX(dotNH, roughness) * dotNH / (4.0 * dotVH) + 0.0001;
			// Slid angle of current smple
			float omegaS = 1.0 / (float(numSamples) * pdf);
			// Solid angle of 1 pixel across all cube faces
			float omegaP = 4.0 * PI / (6.0 * envMapDim * envMapDim);
			// Biased (+1.0) mip level for better result
			float mipLevel = roughness == 0.0 ? 0.0 : max(0.5 * log2(omegaS / omegaP) + 1.0, 0.0f);
			color += textureLod(CubeSampler0, L, mipLevel).rgb * dotNL;
			totalWeight += dotNL;
		}
		*/
	}
	return (color / totalWeight);
}

vec4 CubeRadiance()
{
	vec3 N = get_world_normal();
	N.y = -N.y;
	if (CubeRadianceParam.mode == 0)
	{
		//radiance
		return vec4(prefilterEnvMap(N, 0.5), 1.0);
	}
	else
	{
		// irradiance
		float roughness = float(EvaluationParam.mipmapNumber)/max(float(EvaluationParam.mipmapCount-1), 1.);
		return vec4(prefilterEnvMap(N, roughness), 1.0);
	}
}
