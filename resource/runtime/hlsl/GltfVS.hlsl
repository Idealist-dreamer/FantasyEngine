//=============================================================================
// GltfVS.hlsl - glTF Bindless Vertex Shader
// DirectX 12 Modern Pipeline
// 
// Root Signature Layout:
// [0] Root Constants (b0, space0) - BindlessDrawConstants (8 DWORDs)
// [1] SRV (t0, space0) - MegaBuffer (ByteAddressBuffer)
// [2] CBV (b1, space0) - PerFrameConstants
//=============================================================================

#pragma pack_matrix(row_major)

// Root constants for bindless vertex fetching (32 bytes = 8 DWORDs)
cbuffer BindlessDrawConstants : register(b0) {
    uint g_PosOffset;       // Offset in MegaBuffer for positions
    uint g_PosStride;       // Stride of position attribute
    uint g_NormOffset;      // Offset for normals (~0u if not present)
    uint g_NormStride;
    uint g_TanOffset;       // Offset for tangents
    uint g_TanStride;
    uint g_UVOffset;        // Offset for UVs
    uint g_UVStride;
};

// Per-frame camera constants
cbuffer PerFrameConstants : register(b1) {
    float4x4 g_ViewMatrix;
    float4x4 g_ProjMatrix;
    float4x4 g_ViewProjMatrix;
    float3   g_CameraPosition;
    float    g_Padding;
};

// MegaBuffer containing all vertex data
ByteAddressBuffer MegaBuffer : register(t0);

// Output to pixel shader
struct VS_OUTPUT {
    float4 PositionCS : SV_Position;
    float3 NormalWS   : NORMAL;
    float2 TexCoord   : TEXCOORD0;
    float3 PositionWS : TEXCOORD1;
};

// Helper to load float3 from ByteAddressBuffer with stride
float3 LoadFloat3(uint baseOffset, uint vertexIndex, uint stride) {
    uint offset = baseOffset + vertexIndex * stride;
    
    // ByteAddressBuffer loads require 4-byte alignment
    // Load as uint4 to cover 12 bytes (3 floats)
    uint4 data = uint4(
        MegaBuffer.Load(offset),
        MegaBuffer.Load(offset + 4),
        MegaBuffer.Load(offset + 8),
        MegaBuffer.Load(offset + 12)
    );
    
    return asfloat(data.xyz);
}

// Helper to load float2 from ByteAddressBuffer
float2 LoadFloat2(uint baseOffset, uint vertexIndex, uint stride) {
    uint offset = baseOffset + vertexIndex * stride;
    uint2 data = uint2(
        MegaBuffer.Load(offset),
        MegaBuffer.Load(offset + 4)
    );
    return asfloat(data);
}

//-----------------------------------------------------------------------------
// Main Entry Point
//-----------------------------------------------------------------------------
VS_OUTPUT main(uint VertexID : SV_VertexID) {
    VS_OUTPUT output;
    
    // Load position from MegaBuffer
    float3 position = LoadFloat3(g_PosOffset, VertexID, g_PosStride);
    
    // Transform to clip space
    float4 worldPos = float4(position, 1.0f);
    output.PositionCS = mul(g_ViewProjMatrix, worldPos);
    output.PositionWS = position;
    
    // Load normal if present (indicated by non-~0u offset)
    if (g_NormOffset != 0xFFFFFFFF) {
        float3 normal = LoadFloat3(g_NormOffset, VertexID, g_NormStride);
        // Transform normal to world space (assuming no non-uniform scale)
        output.NormalWS = normalize(mul((float3x3)g_ViewMatrix, normal));
    } else {
        output.NormalWS = float3(0.0f, 1.0f, 0.0f);
    }
    
    // Load UV if present
    if (g_UVOffset != 0xFFFFFFFF) {
        output.TexCoord = LoadFloat2(g_UVOffset, VertexID, g_UVStride);
    } else {
        output.TexCoord = float2(0.0f, 0.0f);
    }
    
    return output;
}
