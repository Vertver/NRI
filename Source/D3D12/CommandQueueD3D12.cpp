// © 2021 NVIDIA Corporation

#include "SharedD3D12.h"

#include "CommandBufferD3D12.h"
#include "CommandQueueD3D12.h"
#include "FenceD3D12.h"
#include "HelperDataUpload.h"
#include "HelperWaitIdle.h"

using namespace nri;

Result CommandQueueD3D12::Create(CommandQueueType queueType) {
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.NodeMask = NRI_NODE_MASK;
    commandQueueDesc.Type = GetCommandListType(queueType);

    HRESULT hr = m_Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_CommandQueue));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandQueue()");

    m_CommandListType = commandQueueDesc.Type;

    return Result::SUCCESS;
}

Result CommandQueueD3D12::Create(ID3D12CommandQueue* commandQueue) {
    const D3D12_COMMAND_QUEUE_DESC& commandQueueDesc = commandQueue->GetDesc();

    m_CommandQueue = commandQueue;
    m_CommandListType = commandQueueDesc.Type;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void CommandQueueD3D12::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.waitFences[i];
        FenceD3D12* fence = (FenceD3D12*)fenceSubmitDesc.fence;
        fence->QueueWait(*this, fenceSubmitDesc.value);
    }

    if (queueSubmitDesc.commandBufferNum) {
        ID3D12CommandList** commandLists = STACK_ALLOC(ID3D12CommandList*, queueSubmitDesc.commandBufferNum);
        for (uint32_t j = 0; j < queueSubmitDesc.commandBufferNum; j++)
            commandLists[j] = *(CommandBufferD3D12*)queueSubmitDesc.commandBuffers[j];

        m_CommandQueue->ExecuteCommandLists(queueSubmitDesc.commandBufferNum, commandLists);
    }

    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.signalFences[i];
        FenceD3D12* fence = (FenceD3D12*)fenceSubmitDesc.fence;
        fence->QueueSignal(*this, fenceSubmitDesc.value);
    }
}

inline Result CommandQueueD3D12::UploadData(
    const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    HelperDataUpload helperDataUpload(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

inline Result CommandQueueD3D12::WaitForIdle() {
    return WaitIdle(m_Device.GetCoreInterface(), (Device&)m_Device, (CommandQueue&)*this);
}

#include "CommandQueueD3D12.hpp"
