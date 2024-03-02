// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "CommandBufferVK.h"
#include "CommandQueueVK.h"

using namespace nri;

Result CommandQueueVK::Create(const CommandQueueVKDesc& commandQueueDesc) {
    m_Handle = (VkQueue)commandQueueDesc.vkQueue;
    m_FamilyIndex = commandQueueDesc.familyIndex;
    m_Type = commandQueueDesc.commandQueueType;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void CommandQueueVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_QUEUE, (uint64_t)m_Handle, name);
}

inline void CommandQueueVK::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    ExclusiveScope lock(m_Lock);

    VkCommandBuffer* commandBuffers = STACK_ALLOC(VkCommandBuffer, queueSubmitDesc.commandBufferNum);
    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++)
        commandBuffers[i] = *(CommandBufferVK*)queueSubmitDesc.commandBuffers[i];

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, queueSubmitDesc.commandBufferNum, commandBuffers, 0, nullptr};

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.QueueSubmit(m_Handle, 1, &submitInfo, VK_NULL_HANDLE);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, ReturnVoid(), "Submit: vkQueueSubmit returned %d", (int32_t)result);
}

inline Result CommandQueueVK::UploadData(
    const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    HelperDataUpload helperDataUpload(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

inline Result CommandQueueVK::WaitForIdle() {
    ExclusiveScope lock(m_Lock);

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.QueueWaitIdle(m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "WaitForIdle: vkQueueWaitIdle returned %d", (int32_t)result);

    return Result::SUCCESS;
}

#include "CommandQueueVK.hpp"