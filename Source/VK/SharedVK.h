// © 2021 NVIDIA Corporation

#pragma once

#include "SharedExternal.h"
#include "DeviceBase.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

#undef CreateSemaphore

#include "DispatchTable.h"

typedef uint16_t MemoryTypeIndexType;
struct MemoryTypeInfo
{
    MemoryTypeIndexType memoryTypeIndex;
    nri::MemoryLocation memoryLocation;
    bool isDedicated;
};
static_assert(sizeof(MemoryTypeInfo) <= sizeof(nri::MemoryType), "Unexpected structure size");

union MemoryTypeUnpack
{
    nri::MemoryType type;
    MemoryTypeInfo info;
};

template< typename HandleType, typename ImplType, typename NRIType >
constexpr HandleType GetVulkanHandle(NRIType* object, uint32_t nodeIndex)
{ return (object != nullptr) ? (*(ImplType*)object).GetHandle(nodeIndex) : HandleType(VK_NULL_HANDLE); }

constexpr bool IsHostVisibleMemory(nri::MemoryLocation location)
{ return location > nri::MemoryLocation::DEVICE; }

constexpr bool IsHostMemory(nri::MemoryLocation location)
{ return location > nri::MemoryLocation::DEVICE_UPLOAD; }

// TODO: mostly needed for AMD?
#if 1
    #define IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    #define IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
#else
    #define IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL VK_IMAGE_LAYOUT_GENERAL
    #define IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL VK_IMAGE_LAYOUT_GENERAL
#endif

#include "ConversionVK.h"
#include "DeviceVK.h"
