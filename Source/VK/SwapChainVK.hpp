/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

static void NRI_CALL SetSwapChainDebugName(SwapChain& swapChain, const char* name)
{
    ((SwapChainVK&)swapChain).SetDebugName(name);
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum)
{
    return ((SwapChainVK&)swapChain).GetTextures(textureNum);
}

static uint32_t NRI_CALL AcquireNextSwapChainTexture(SwapChain& swapChain)
{
    return ((SwapChainVK&)swapChain).AcquireNextTexture();
}

static Result NRI_CALL SwapChainPresent(SwapChain& swapChain)
{
    return ((SwapChainVK&)swapChain).Present();
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc)
{
    return ((SwapChainVK&)swapChain).GetDisplayDesc(displayDesc);
}

Define_SwapChain_PartiallyFillFunctionTable(VK)
