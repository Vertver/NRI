// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "DescriptorPoolVK.h"
#include "DescriptorSetVK.h"
#include "PipelineLayoutVK.h"

using namespace nri;

DescriptorPoolVK::~DescriptorPoolVK() {
    const auto& lowLevelAllocator = m_Device.GetStdAllocator().GetInterface();

    for (size_t i = 0; i < m_AllocatedSets.size(); i++) {
        m_AllocatedSets[i]->~DescriptorSetVK();
        lowLevelAllocator.Free(lowLevelAllocator.userArg, m_AllocatedSets[i]);
    }

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Handle != VK_NULL_HANDLE && m_OwnsNativeObjects)
        vk.DestroyDescriptorPool(m_Device, m_Handle, m_Device.GetAllocationCallbacks());
}

inline void AddDescriptorPoolSize(VkDescriptorPoolSize* poolSizeArray, uint32_t& poolSizeArraySize, VkDescriptorType type, uint32_t descriptorCount) {
    if (descriptorCount == 0)
        return;

    VkDescriptorPoolSize& poolSize = poolSizeArray[poolSizeArraySize++];
    poolSize.type = type;
    poolSize.descriptorCount = descriptorCount;
}

Result DescriptorPoolVK::Create(const DescriptorPoolDesc& descriptorPoolDesc) {
    m_OwnsNativeObjects = true;

    const auto& vk = m_Device.GetDispatchTable();

    VkDescriptorPoolSize descriptorPoolSizeArray[16] = {};
    for (uint32_t i = 0; i < GetCountOf(descriptorPoolSizeArray); i++)
        descriptorPoolSizeArray[i].type = (VkDescriptorType)i;

    uint32_t poolSizeCount = 0;
    AddDescriptorPoolSize(descriptorPoolSizeArray, poolSizeCount, VK_DESCRIPTOR_TYPE_SAMPLER, descriptorPoolDesc.samplerMaxNum);
    AddDescriptorPoolSize(descriptorPoolSizeArray, poolSizeCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorPoolDesc.constantBufferMaxNum);
    AddDescriptorPoolSize(descriptorPoolSizeArray, poolSizeCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, descriptorPoolDesc.dynamicConstantBufferMaxNum);
    AddDescriptorPoolSize(descriptorPoolSizeArray, poolSizeCount, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorPoolDesc.textureMaxNum);
    AddDescriptorPoolSize(descriptorPoolSizeArray, poolSizeCount, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptorPoolDesc.storageTextureMaxNum);
    AddDescriptorPoolSize(descriptorPoolSizeArray, poolSizeCount, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, descriptorPoolDesc.bufferMaxNum);
    AddDescriptorPoolSize(descriptorPoolSizeArray, poolSizeCount, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, descriptorPoolDesc.storageBufferMaxNum);
    AddDescriptorPoolSize(
        descriptorPoolSizeArray, poolSizeCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorPoolDesc.structuredBufferMaxNum + descriptorPoolDesc.storageStructuredBufferMaxNum);
    AddDescriptorPoolSize(descriptorPoolSizeArray, poolSizeCount, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, descriptorPoolDesc.accelerationStructureMaxNum);

    const VkDescriptorPoolCreateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        descriptorPoolDesc.descriptorSetMaxNum, poolSizeCount, descriptorPoolSizeArray};

    VkResult result = vk.CreateDescriptorPool(m_Device, &info, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateDescriptorPool returned %d", (int32_t)result);

    return Result::SUCCESS;
}

Result DescriptorPoolVK::Create(const DescriptorPoolVKDesc& descriptorPoolVKDesc) {
    m_OwnsNativeObjects = false;
    m_Handle = (VkDescriptorPool)descriptorPoolVKDesc.vkDescriptorPool;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void DescriptorPoolVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t)m_Handle, name);
}

inline Result DescriptorPoolVK::AllocateDescriptorSets(
    const PipelineLayout& pipelineLayout, uint32_t setIndexInPipelineLayout, DescriptorSet** descriptorSets, uint32_t numberOfCopies, uint32_t variableDescriptorNum) {
    const PipelineLayoutVK& pipelineLayoutVK = (const PipelineLayoutVK&)pipelineLayout;

    const uint32_t freeSetNum = (uint32_t)m_AllocatedSets.size() - m_UsedSets;

    if (freeSetNum < numberOfCopies) {
        const uint32_t newSetNum = numberOfCopies - freeSetNum;
        const uint32_t prevSetNum = (uint32_t)m_AllocatedSets.size();
        m_AllocatedSets.resize(prevSetNum + newSetNum);

        const auto& lowLevelAllocator = m_Device.GetStdAllocator().GetInterface();

        for (size_t i = 0; i < newSetNum; i++) {
            m_AllocatedSets[prevSetNum + i] = (DescriptorSetVK*)lowLevelAllocator.Allocate(lowLevelAllocator.userArg, sizeof(DescriptorSetVK), alignof(DescriptorSetVK));
            Construct(m_AllocatedSets[prevSetNum + i], 1, m_Device);
        }
    }

    for (size_t i = 0; i < numberOfCopies; i++)
        descriptorSets[i] = (DescriptorSet*)m_AllocatedSets[m_UsedSets + i];
    m_UsedSets += numberOfCopies;

    const VkDescriptorSetLayout setLayout = pipelineLayoutVK.GetDescriptorSetLayout(setIndexInPipelineLayout);
    const DescriptorSetDesc& setDesc = pipelineLayoutVK.GetRuntimeBindingInfo().descriptorSetDescs[setIndexInPipelineLayout];
    const bool hasVariableDescriptorNum = pipelineLayoutVK.GetRuntimeBindingInfo().hasVariableDescriptorNum[setIndexInPipelineLayout];

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO};
    variableDescriptorCountInfo.descriptorSetCount = 1;
    variableDescriptorCountInfo.pDescriptorCounts = &variableDescriptorNum;

    VkDescriptorSetAllocateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    info.pNext = hasVariableDescriptorNum ? &variableDescriptorCountInfo : nullptr;
    info.descriptorPool = m_Handle;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &setLayout;

    const auto& vk = m_Device.GetDispatchTable();
    for (uint32_t i = 0; i < numberOfCopies; i++) {
        VkDescriptorSet handle = VK_NULL_HANDLE;
        VkResult result = vk.AllocateDescriptorSets(m_Device, &info, &handle);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkAllocateDescriptorSets returned %d", (int32_t)result);

        ((DescriptorSetVK*)descriptorSets[i])->Create(handle, setDesc);
    }

    return Result::SUCCESS;
}

inline void DescriptorPoolVK::Reset() {
    m_UsedSets = 0;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.ResetDescriptorPool(m_Device, m_Handle, (VkDescriptorPoolResetFlags)0);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, ReturnVoid(), "vkResetDescriptorPool returned %d", (int32_t)result);
}

#include "DescriptorPoolVK.hpp"