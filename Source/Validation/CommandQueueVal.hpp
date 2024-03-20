// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetCommandQueueDebugName(CommandQueue& commandQueue, const char* name) {
    ((CommandQueueVal&)commandQueue).SetDebugName(name);
}

static void NRI_CALL QueueSubmit(CommandQueue& commandQueue, const QueueSubmitDesc& queueSubmitDesc) {
    ((CommandQueueVal&)commandQueue).Submit(queueSubmitDesc, nullptr);
}

#pragma endregion

#pragma region[  Helper  ]

static Result NRI_CALL UploadData(CommandQueue& commandQueue, const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs,
    uint32_t bufferUploadDescNum) {
    return ((CommandQueueVal&)commandQueue).UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static Result NRI_CALL WaitForIdle(CommandQueue& commandQueue) {
    if (!(&commandQueue))
        return Result::SUCCESS;

    return ((CommandQueueVal&)commandQueue).WaitForIdle();
}

#pragma endregion

#pragma region[  Low latency  ]

static void NRI_CALL QueueSubmitTrackable(CommandQueue& commandQueue, const QueueSubmitDesc& workSubmissionDesc, const SwapChain& swapChain) {
    ((CommandQueueVal&)commandQueue).Submit(workSubmissionDesc, &swapChain);
}

#pragma endregion

Define_Core_CommandQueue_PartiallyFillFunctionTable(Val);
Define_Helper_CommandQueue_PartiallyFillFunctionTable(Val);
Define_LowLatency_CommandQueue_PartiallyFillFunctionTable(Val);
