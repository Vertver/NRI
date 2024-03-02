// © 2021 NVIDIA Corporation

#pragma once

#include <d3d11_4.h>

#include "SharedExternal.h"

struct AGSContext;

struct ID3D11DeviceContext4;
typedef ID3D11DeviceContext4 ID3D11DeviceContextBest;

#define SHADER_EXT_UAV_SLOT 63 // TODO: D3D 11.1 assumed

namespace nri {

struct D3D11Extensions;

constexpr Mip_t NULL_TEXTURE_REGION_DESC = Mip_t(-1);

enum class BufferType {
    DEVICE,
    DYNAMIC,
    READBACK,
    UPLOAD
};

enum class MapType {
    DEFAULT,
    READ
};

enum class DescriptorTypeDX11 : uint8_t {
    // don't change order
    NO_SHADER_VISIBLE,
    RESOURCE,
    SAMPLER,
    STORAGE,
    // must be last!
    CONSTANT,
    DYNAMIC_CONSTANT
};

D3D11_PRIMITIVE_TOPOLOGY GetD3D11TopologyFromTopology(Topology topology, uint32_t patchPoints);
D3D11_CULL_MODE GetD3D11CullModeFromCullMode(CullMode cullMode);
D3D11_COMPARISON_FUNC GetD3D11ComparisonFuncFromCompareFunc(CompareFunc compareFunc);
D3D11_STENCIL_OP GetD3D11StencilOpFromStencilFunc(StencilFunc stencilFunc);
D3D11_BLEND_OP GetD3D11BlendOpFromBlendFunc(BlendFunc blendFunc);
D3D11_BLEND GetD3D11BlendFromBlendFactor(BlendFactor blendFactor);
D3D11_LOGIC_OP GetD3D11LogicOpFromLogicFunc(LogicFunc logicalFunc);
bool GetTextureDesc(const TextureD3D11Desc& textureD3D11Desc, TextureDesc& textureDesc);
bool GetBufferDesc(const BufferD3D11Desc& bufferD3D11Desc, BufferDesc& bufferDesc);

struct SubresourceInfo {
    const void* resource = nullptr;
    uint64_t data = 0;

    inline void Initialize(const void* tex, Mip_t mipOffset, Mip_t mipNum, Dim_t arrayOffset, Dim_t arraySize) {
        resource = tex;
        data = (uint64_t(arraySize) << 48) | (uint64_t(arrayOffset) << 32) | (uint64_t(mipNum) << 16) | uint64_t(mipOffset);
    }

    inline void Initialize(const void* buf) {
        resource = buf;
        data = 0;
    }

    friend bool operator==(const SubresourceInfo& a, const SubresourceInfo& b) {
        return a.resource == b.resource && a.data == b.data;
    }
};

struct SubresourceAndSlot {
    SubresourceInfo subresource;
    uint32_t slot;
};

struct BindingState {
    std::vector<SubresourceAndSlot> resources; // max expected size - D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT
    std::vector<SubresourceAndSlot> storages;  // max expected size - D3D11_1_UAV_SLOT_COUNT
    std::array<ID3D11UnorderedAccessView*, D3D11_PS_CS_UAV_REGISTER_COUNT> graphicsStorageDescriptors = {};

    inline void TrackSubresource_UnbindIfNeeded_PostponeGraphicsStorageBinding(
        ID3D11DeviceContextBest* deferredContext, const SubresourceInfo& subresource, void* descriptor, uint32_t slot, bool isGraphics, bool isStorage) {
        constexpr void* null = nullptr;

        if (isStorage) {
            for (uint32_t i = 0; i < (uint32_t)resources.size(); i++) {
                const SubresourceAndSlot& subresourceAndSlot = resources[i];
                if (subresourceAndSlot.subresource == subresource) {
                    // TODO: store visibility to unbind only for necessary stages
                    deferredContext->VSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
                    deferredContext->HSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
                    deferredContext->DSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
                    deferredContext->GSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
                    deferredContext->PSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
                    deferredContext->CSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);

                    resources[i] = resources.back();
                    resources.pop_back();
                    i--;
                }
            }

            storages.push_back({subresource, slot});

            if (isGraphics)
                graphicsStorageDescriptors[slot] = (ID3D11UnorderedAccessView*)descriptor;
        } else {
            for (uint32_t i = 0; i < (uint32_t)storages.size(); i++) {
                const SubresourceAndSlot& subresourceAndSlot = storages[i];
                if (subresourceAndSlot.subresource == subresource) {
                    deferredContext->CSSetUnorderedAccessViews(subresourceAndSlot.slot, 1, (ID3D11UnorderedAccessView**)&null, nullptr);

                    graphicsStorageDescriptors[subresourceAndSlot.slot] = nullptr;

                    storages[i] = storages.back();
                    storages.pop_back();
                    i--;
                }
            }

            resources.push_back({subresource, slot});
        }
    }

    inline void UnbindAndReset(ID3D11DeviceContextBest* deferredContext) {
        constexpr void* null = nullptr;

        for (const SubresourceAndSlot& subresourceAndSlot : resources) {
            // TODO: store visibility to unbind only for necessary stages
            deferredContext->VSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
            deferredContext->HSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
            deferredContext->DSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
            deferredContext->GSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
            deferredContext->PSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
            deferredContext->CSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
        }
        resources.clear();

        if (!storages.empty())
            deferredContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, 0, 0, nullptr, nullptr);
        for (const SubresourceAndSlot& subresourceAndSlot : storages)
            deferredContext->CSSetUnorderedAccessViews(subresourceAndSlot.slot, 1, (ID3D11UnorderedAccessView**)&null, nullptr);
        storages.clear();

        memset(&graphicsStorageDescriptors, 0, sizeof(graphicsStorageDescriptors));
    }
};

struct CommandBufferHelper {
    virtual ~CommandBufferHelper() {
    }
    virtual Result Create(ID3D11DeviceContext* precreatedContext) = 0;
    virtual void Submit() = 0;
    virtual ID3D11DeviceContext* GetNativeObject() const = 0;
    virtual StdAllocator<uint8_t>& GetStdAllocator() const = 0;
};

static inline uint64_t ComputeHash(const void* key, uint32_t len) {
    const uint8_t* p = (uint8_t*)key;
    uint64_t result = 14695981039346656037ull;
    while (len--)
        result = (result ^ (*p++)) * 1099511628211ull;

    return result;
}

struct SamplePositionsState {
    std::array<SamplePosition, 32> positions;
    uint64_t positionHash;
    Sample_t positionNum;

    inline void Reset() {
        memset(&positions, 0, sizeof(positions));
        positionNum = 0;
        positionHash = 0;
    }

    inline void Set(const SamplePosition* samplePositions, Sample_t samplePositionNum) {
        const uint32_t size = sizeof(SamplePosition) * samplePositionNum;

        memcpy(&positions, samplePositions, size);
        positionHash = ComputeHash(samplePositions, size);
        positionNum = samplePositionNum;
    }
};

} // namespace nri

#include "D3D11Extensions.h"
#include "DeviceD3D11.h"
