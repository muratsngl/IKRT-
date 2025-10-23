#version 460 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
    vec4 FragPosDirectionalLightSpace;  // For directional light
    vec4 FragPosLightSpace[10];  // Array for up to 10 spotlights
} fs_in;

// Material property textures
uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D normalMap;
uniform sampler2D aoMap;

// Shadow maps
uniform sampler2D directionalShadowMap;     // For directional light
uniform sampler2D shadowMaps[10];           // For spotlights
uniform samplerCube shadowCubemaps[10];     // For point lights
uniform int numActiveShadowCastingSpotLights;
uniform int numActiveShadowCastingPointLights;
uniform bool directionalLightCastsShadow;
// Light struct definitions
struct PointLight {
    vec3 position;
    float intensity;
    vec3 color;
    float _padding; // Not needed in GLSL, but layout is identical
};

struct SpotLight {
    vec3 position;
    float intensity;
    vec3 direction;
    float innerCone;
    vec3 color;
    float outerCone;
};

// Lights UBO - properly aligned structs
layout(std140, binding = 4) uniform LightData {
    // Directional Light
    vec3 directionalLightDirection;  // offset 0
    float directionalLightIntensity; // offset 12
    vec3 directionalLightColor;      // offset 16
    float _padding1;                 // offset 28
    
    // Light Arrays
    PointLight pointLights[10];      // offset 32, size 320
    SpotLight spotLights[10];        // offset 352, size 480
    
    // Light Counts
    int numPointLights;              // offset 832
    int numSpotLights;               // offset 836
} lights;

// Camera
uniform vec3 camPos;
uniform float far_plane;


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
// Shadow Calculation Functions
// ----------------------------------------------------------------------------

// Shadow calculation for directional light (uses dedicated texture)
float DirectionalShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get closest depth value from light's perspective
    float closestDepth = texture(directionalShadowMap, projCoords.xy).r; 
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Calculate bias based on surface angle
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF (Percentage Closer Filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(directionalShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(directionalShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

// Shadow calculation for spotlights (uses array texture)
float ShadowCalculation(int lightIndex, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMaps[lightIndex], projCoords.xy).r; 
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Calculate bias based on surface angle
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // Check whether current frag pos is in shadow
    // PCF (Percentage Closer Filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMaps[lightIndex], 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMaps[lightIndex], projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

// ----------------------------------------------------------------------------
// Cubemap Shadow Calculation for Point Lights
// ----------------------------------------------------------------------------
float ShadowCalculationCubemap(int lightIndex, vec3 fragPos, vec3 lightPos)
{
    // Get vector between fragment position and light position
    vec3 fragToLight = fragPos - lightPos;
    
    // Use the light to fragment vector to sample from the depth map    
    float closestDepth = texture(shadowCubemaps[lightIndex], fragToLight).r;
    
    // Transform to original depth value (closestDepth is in [0,1], so multiply by far_plane)
    closestDepth *= far_plane;
    
    // Get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    
    // Test for shadows with a bias
    float bias = 0.05;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    return shadow;
}

// ----------------------------------------------------------------------------
// Light Calculation Functions
// ----------------------------------------------------------------------------

// Calculate directional light contribution
vec3 calculateDirectionalLight(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3 L = normalize(-lights.directionalLightDirection);
    vec3 H = normalize(V + L);
    vec3 radiance = lights.directionalLightColor * lights.directionalLightIntensity;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);       
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// Calculate point light contribution
vec3 calculatePointLight(int index, vec3 N, vec3 V, vec3 FragPos, vec3 albedo, float metallic, float roughness, vec3 F0, float shadow)
{
    vec3 L = normalize(lights.pointLights[index].position - FragPos);
    vec3 H = normalize(V + L);
    float distance = length(lights.pointLights[index].position - FragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lights.pointLights[index].color * lights.pointLights[index].intensity * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);       
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    
    // Apply shadow
    return (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
}

// Calculate spot light contribution
vec3 calculateSpotLight(int index, vec3 N, vec3 V, vec3 FragPos, vec3 albedo, float metallic, float roughness, vec3 F0, float shadow)
{
    vec3 L = normalize(lights.spotLights[index].position - FragPos);
    vec3 H = normalize(V + L);
    float distance = length(lights.spotLights[index].position - FragPos);
    
    // Spot light cone calculation
    vec3 lightDir = normalize(lights.spotLights[index].direction);
    float theta = dot(L, normalize(-lightDir));
    float epsilon = lights.spotLights[index].innerCone - lights.spotLights[index].outerCone;
    float intensity = clamp((theta - lights.spotLights[index].outerCone) / epsilon, 0.0, 1.0);
    
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lights.spotLights[index].color * lights.spotLights[index].intensity * attenuation * intensity;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);       
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    
    // Apply shadow
    return (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
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
    
    // Directional light with shadows
    vec3 dirLightContribution = calculateDirectionalLight(N, V, albedo, metallic, roughness, F0);
    if(directionalLightCastsShadow) {
        vec3 lightDir = normalize(-lights.directionalLightDirection);
        float shadow = DirectionalShadowCalculation(fs_in.FragPosDirectionalLightSpace, N, lightDir);
        dirLightContribution *= (1.0 - shadow);
    }
    Lo += dirLightContribution;
    
    // Point lights with shadows
    for(int i = 0; i < lights.numPointLights && i < 10; ++i) {
        float shadow = 0.0;
        // Only calculate shadow if this light casts shadows
        if(i < numActiveShadowCastingPointLights) {
            shadow = ShadowCalculationCubemap(i, fs_in.FragPos, lights.pointLights[i].position);
        }
        Lo += calculatePointLight(i, N, V, fs_in.FragPos, albedo, metallic, roughness, F0, shadow);
    }
    
    // Spot lights with shadows
    for(int i = 0; i < lights.numSpotLights && i < 10; ++i) {
        float shadow = 0.0;
        // Only calculate shadow if this light casts shadows
        if(i < numActiveShadowCastingSpotLights) {
            vec3 lightDir = normalize(lights.spotLights[i].position - fs_in.FragPos);
            shadow = ShadowCalculation(i, fs_in.FragPosLightSpace[i], N, lightDir);
        }
        Lo += calculateSpotLight(i, N, V, fs_in.FragPos, albedo, metallic, roughness, F0, shadow);
    }

    // 3. Calculate ambient lighting
    // --------------------------------------------------
    // This is a simple ambient term. For full PBR, this would be replaced with Image-Based Lighting (IBL).
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
	
    // 4. Final Color Correction
    // --------------------------------------------------
    // HDR tone mapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  

    FragColor = vec4(color, 1.0);
}