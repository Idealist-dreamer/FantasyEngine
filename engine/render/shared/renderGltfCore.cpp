#include "renderGltfCore.h"

#include "tiny_gltf.h"

#include "engine/render/rhi/GraphicsCore.h"
#include "engine/render/rhi/GpuBuffer.h"
#include "engine/render/rhi/CommandContext.h"

namespace fe::engine::render {

// ============================================================================
// 内部私有数据结构 (对外完全隐藏)
// ============================================================================
__declspec(align(16)) struct BindlessDrawConstants {
  uint32_t posOffset = ~0u, posStride = 0;
  uint32_t normOffset = ~0u, normStride = 0;
  uint32_t tanOffset = ~0u, tanStride = 0;
  uint32_t uvOffset = ~0u, uvStride = 0;
};

struct RenderPrimitive {
  uint32_t indexCount = 0;
  uint32_t vertexCount = 0;

  D3D12_INDEX_BUFFER_VIEW indexBufferView{};
  BindlessDrawConstants drawArgs{};

  int materialIndex = -1;
};

struct RenderMesh {
  stl::vector<RenderPrimitive> primitives;
};

struct RenderModel {
  uint32_t id;
  stl::shared_ptr<ByteAddressBuffer> megaBuffer;  // 唯一的大缓冲
  stl::vector<RenderMesh> meshes;
};

// ============================================================================
// 真正的私有实现类 (Impl)
// ============================================================================
struct RenderGltfCore::Impl {
  static inline uint32_t s_next_id = 0;

  stl::unordered_map<uint32_t, stl::unique_ptr<tinygltf::Model>> model_map;
  stl::unordered_map<uint32_t, stl::unique_ptr<RenderModel>> render_models;

  // 内部辅助方法
  void build_render_data(uint32_t model_id) {
    const tinygltf::Model& gltf = *model_map[model_id];
    auto renderModel = stl::make_unique<RenderModel>();
    renderModel->id = model_id;

    // --- 1. 提纯并构建 Mega Buffer ---
    stl::unordered_set<int> reqViews;
    for (const auto& mesh : gltf.meshes) {
      for (const auto& prim : mesh.primitives) {
        if (prim.indices >= 0)
          reqViews.insert(gltf.accessors[prim.indices].bufferView);
        for (const auto& [name, accIdx] : prim.attributes)
          reqViews.insert(gltf.accessors[accIdx].bufferView);
      }
    }

    uint32_t totalSize = 0;
    stl::unordered_map<int, uint32_t> viewToMegaOffset;
    for (int vIdx : reqViews) {
      viewToMegaOffset[vIdx] = totalSize;
      totalSize += (static_cast<uint32_t>(gltf.bufferViews[vIdx].byteLength) + 3) & ~3;  // 4字节对齐
    }

    stl::vector<uint8_t> cpuData(totalSize, 0);
    for (int vIdx : reqViews) {
      const auto& view = gltf.bufferViews[vIdx];
      memcpy(cpuData.data() + viewToMegaOffset[vIdx], gltf.buffers[view.buffer].data.data() + view.byteOffset, view.byteLength);
    }

    renderModel->megaBuffer = stl::make_shared<ByteAddressBuffer>();
    renderModel->megaBuffer->Create(L"GLTF_MegaBuffer", totalSize / 4, 4, cpuData.data());
    D3D12_GPU_VIRTUAL_ADDRESS baseAddr = renderModel->megaBuffer->GetGpuVirtualAddress();

    // --- 2. 解析网格及常量属性 ---
    for (const auto& gltfMesh : gltf.meshes) {
      RenderMesh rMesh;
      for (const auto& gltfPrim : gltfMesh.primitives) {
        RenderPrimitive rPrim;
        rPrim.materialIndex = gltfPrim.material;

        // 解析 Index Buffer
        if (gltfPrim.indices >= 0) {
          const auto& acc = gltf.accessors[gltfPrim.indices];
          rPrim.indexCount = static_cast<uint32_t>(acc.count);
          rPrim.indexBufferView.BufferLocation = baseAddr + viewToMegaOffset[acc.bufferView] + acc.byteOffset;
          rPrim.indexBufferView.Format = (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
          rPrim.indexBufferView.SizeInBytes = rPrim.indexCount * tinygltf::GetComponentSizeInBytes(acc.componentType);
        }

        // 提取属性的 Lambda 闭包
        auto extract_attr = [&](const char* name, uint32_t& outOffset, uint32_t& outStride) {
          if (gltfPrim.attributes.count(name)) {
            const auto& acc = gltf.accessors[gltfPrim.attributes.at(name)];
            const auto& view = gltf.bufferViews[acc.bufferView];

            outOffset = viewToMegaOffset[acc.bufferView] + acc.byteOffset;
            uint32_t elementSize = tinygltf::GetComponentSizeInBytes(acc.componentType) * tinygltf::GetNumComponentsInType(acc.type);
            outStride = view.byteStride > 0 ? static_cast<uint32_t>(view.byteStride) : elementSize;

            if (strcmp(name, "POSITION") == 0)
              rPrim.vertexCount = static_cast<uint32_t>(acc.count);
          }
        };

        // 按需提取需要的 4 个属性
        extract_attr("POSITION", rPrim.drawArgs.posOffset, rPrim.drawArgs.posStride);
        extract_attr("NORMAL", rPrim.drawArgs.normOffset, rPrim.drawArgs.normStride);
        extract_attr("TANGENT", rPrim.drawArgs.tanOffset, rPrim.drawArgs.tanStride);
        extract_attr("TEXCOORD_0", rPrim.drawArgs.uvOffset, rPrim.drawArgs.uvStride);

        rMesh.primitives.push_back(rPrim);
      }
      renderModel->meshes.push_back(rMesh);
    }

    render_models[model_id] = stl::move(renderModel);
  }
};

// ============================================================================
// RenderGltfCore 接口实现
// ============================================================================

RenderGltfCore::RenderGltfCore(){FE_DECLARE_PRIVATE_INIT}

RenderGltfCore::~RenderGltfCore() = default;

void RenderGltfCore::load_gltf(const stl::string& file) {
  tinygltf::TinyGLTF loader;
  std::string err, warn;  // tinygltf 必须使用 std::string
  auto gltfModel = stl::make_unique<tinygltf::Model>();

  if (loader.LoadBinaryFromFile(gltfModel.get(), &err, &warn, file.c_str())) {
    auto& model_map = d()->model_map;

    uint32_t id = d()->s_next_id++;
    model_map[id] = stl::move(gltfModel);

    // 构建渲染数据
    d()->build_render_data(id);
  } else {
    // 处理错误: err, warn
  }
}

void RenderGltfCore::render(GraphicsContext* gfxContext, uint32_t model_id) {
  auto& render_models = d()->render_models;

  auto it = render_models.find(model_id);
  if (it == render_models.end())
    return;

  const auto& model = *it->second;
  if (!model.megaBuffer)
    return;

  // 1. 全局绑定一次 Mega Buffer 为 SRV
  gfxContext->SetBufferSRV(1, *model.megaBuffer);

  // 2. 极速渲染循环
  for (const auto& mesh : model.meshes) {
    for (const auto& prim : mesh.primitives) {

      // 瞬间提交拉取常量 (零开销)
      gfxContext->SetDynamicConstantBufferView(0, sizeof(BindlessDrawConstants), &prim.drawArgs);

      if (prim.indexCount > 0) {
        gfxContext->SetIndexBuffer(prim.indexBufferView);
        gfxContext->DrawIndexedInstanced(prim.indexCount, 1, 0, 0, 0);
      } else {
        gfxContext->DrawInstanced(prim.vertexCount, 1, 0, 0);
      }
    }
  }
}

}  // namespace fe::engine::render