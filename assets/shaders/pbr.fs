#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

// Material property textures
uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D normalMap;
uniform sampler2D aoMap;

// Lights
uniform vec3 lightPositions[5];
uniform vec3 lightColors[5];

// Camera
uniform vec3 camPos;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// PBR Functions from LearnOpenGL
// ----------------------------------------------------------------------------

// 1. Normal Distribution Function (D): Estimates the amount of microfacets aligned with the halfway vector.
// GGX/Trowbridge-Reitz is a popular and realistic choice.
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

// 2. Geometry Function (G): Describes the self-shadowing property of microfacets.
// Smith's method with Schlick-GGX is used here.
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

// 3. Fresnel Equation (F): Describes the ratio of light that gets reflected vs. refracted.
// The Schlick approximation is used for performance.
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------------------------

void main()
{		
    // 1. Get material properties from textures
    // --------------------------------------------------
    // We assume textures are in sRGB space, so we convert them to linear space for correct lighting calculations.
    vec3 albedo     = pow(texture(albedoMap, fs_in.TexCoords).rgb, vec3(2.2));
    float metallic  = texture(metallicMap, fs_in.TexCoords).r;
    float roughness = texture(roughnessMap, fs_in.TexCoords).r;
    float ao        = texture(aoMap, fs_in.TexCoords).r;

    // Get the normal from the normal map and transform it to world space using the TBN matrix
    vec3 N = texture(normalMap, fs_in.TexCoords).rgb;
    N = normalize(N * 2.0 - 1.0);   
    N = normalize(fs_in.TBN * N);

    // Get view direction
    vec3 V = normalize(camPos - fs_in.FragPos);

    // The base reflectivity (F0). For non-metals, this is low (around 0.04).
    // For metals, F0 is the albedo color. We use `mix` to blend between these two based on the metallic value.
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // 2. Calculate direct lighting contribution
    // --------------------------------------------------
    vec3 Lo = vec3(0.0); // Outgoing radiance
    for(int i = 0; i < 5; ++i) 
    {
        // Calculate per-light vectors
        vec3 L = normalize(lightPositions[i] - fs_in.FragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - fs_in.FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 numerator    = NDF * G * F;
        // Add a small epsilon to the denominator to prevent division by zero
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
        vec3 specular = numerator / denominator;
        
        // kS is the energy that is reflected (specular), kD is the energy that is refracted (diffuse)
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        // We don't want any diffuse light for pure metals
        kD *= 1.0 - metallic;	  
        
        float NdotL = max(dot(N, L), 0.0);        
        
        // Add to outgoing radiance
        // Note: we multiply the diffuse part by albedo. The specular part is already colored by the Fresnel term (F).
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // 3. Calculate ambient lighting
    // --------------------------------------------------
    // This is a simple ambient term. For full PBR, this would be replaced with Image-Based Lighting (IBL).
    vec3 ambient = vec3(1) * albedo * ao;
    vec3 color = ambient + Lo;
	
    // 4. Final Color Correction
    // --------------------------------------------------
    // HDR tone mapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  

    FragColor = vec4(color, 1.0);
}