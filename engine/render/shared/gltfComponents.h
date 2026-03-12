// #pragma once

// #include "foundation/container/stl.h"
// #include <DirectXMath.h>

// namespace fe::engine::render {

// using namespace DirectX;

// /// ECS component for glTF model rendering
// /// Follows data-oriented design: minimal data, cache-friendly
// struct GltfModelComponent {
//   uint32_t modelId = 0xFFFFFFFF;  // Handle to loaded model in RenderGltfCore
//   uint32_t _padding = 0;          // 16-byte alignment

//   XMFLOAT4X4 sceneMatrix;   // Scene transform
//   XMFLOAT4X4 normalMatrix;  // Inverse transpose for normals

//   GltfModelComponent() {
//     XMStoreFloat4x4(&sceneMatrix, XMMatrixIdentity());
//     XMStoreFloat4x4(&normalMatrix, XMMatrixIdentity());
//   }

//   GltfModelComponent(uint32_t id, XMMATRIX scene) : modelId(id) {
//     XMStoreFloat4x4(&sceneMatrix, scene);
//     XMMATRIX invTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, scene));
//     XMStoreFloat4x4(&normalMatrix, invTranspose);
//   }
// };

// /// Transform component for entity positioning
// struct TransformComponent {
//   XMFLOAT3 position{0.0f, 0.0f, 0.0f};
//   XMFLOAT4 rotation{0.0f, 0.0f, 0.0f, 1.0f};  // Quaternion
//   XMFLOAT3 scale{1.0f, 1.0f, 1.0f};

//   XMMATRIX GetSceneMatrix() const {
//     XMVECTOR pos = XMLoadFloat3(&position);
//     XMVECTOR rot = XMLoadFloat4(&rotation);
//     XMVECTOR scl = XMLoadFloat3(&scale);
//     return XMMatrixAffineTransformation(scl, XMQuaternionIdentity(), rot, pos);
//   }
// };

// }  // namespace fe::engine::render
