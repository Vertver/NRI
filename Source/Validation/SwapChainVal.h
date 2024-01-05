/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

namespace nri {

struct TextureVal;

struct SwapChainVal : public DeviceObjectVal<SwapChain> {
    SwapChainVal(DeviceVal& device, SwapChain* swapChain, const SwapChainDesc& swapChainDesc)
        : DeviceObjectVal(device, swapChain), m_SwapChainAPI(device.GetSwapChainInterface()), m_Textures(device.GetStdAllocator()), m_SwapChainDesc(swapChainDesc) {
    }

    ~SwapChainVal();

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);
    Texture* const* GetTextures(uint32_t& textureNum) const;
    uint32_t AcquireNextTexture();
    Result Present();
    Result GetDisplayDesc(DisplayDesc& displayDesc) const;

  private:
    const SwapChainInterface& m_SwapChainAPI;
    mutable Vector<TextureVal*> m_Textures;
    SwapChainDesc m_SwapChainDesc = {};
};

} // namespace nri
