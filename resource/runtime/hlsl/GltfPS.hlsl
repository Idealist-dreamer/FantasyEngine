//=============================================================================
// GltfPS.hlsl - glTF Pixel Shader
// DirectX 12 Modern Pipeline
// 
// Simple forward rendering with basic lighting
//=============================================================================

#pragma pack_matrix(row_major)

// Per-frame camera constants (same as VS)
cbuffer PerFrameConstants : register(b1) {
    float4x4 g_ViewMatrix;
    float4x4 g_ProjMatrix;
    float4x4 g_ViewProjMatrix;
    float3   g_CameraPosition;
    float    g_Padding;
};

// Input from vertex shader
struct PS_INPUT {
    float4 PositionCS : SV_Position;
    float3 NormalWS   : NORMAL;
    float2 TexCoord   : TEXCOORD0;
    float3 PositionWS : TEXCOORD1;
};

//-----------------------------------------------------------------------------
// Main Entry Point
//-----------------------------------------------------------------------------
float4 main(PS_INPUT input) : SV_Target {
    // Normalize interpolated normal
    float3 N = normalize(input.NormalWS);
    
    // Simple directional light (from above-front)
    float3 lightDir = normalize(float3(0.3f, 1.0f, -0.5f));
    float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    
    // Diffuse lighting
    float NdotL = max(dot(N, lightDir), 0.0f);
    float3 diffuse = lightColor * NdotL;
    
    // Simple ambient
    float3 ambient = float3(0.15f, 0.15f, 0.18f);
    
    // Base color (placeholder - would come from material texture)
    float3 baseColor = float3(0.8f, 0.8f, 0.8f);
    
    // Final color
    float3 finalColor = baseColor * (ambient + diffuse);
    
    // Gamma correction (assuming sRGB render target)
    finalColor = pow(finalColor, 1.0f / 2.2f);
    
    return float4(finalColor, 1.0f);
}
