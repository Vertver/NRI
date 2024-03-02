// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct PipelineVK;
struct PipelineLayoutVK;
struct TextureVK;

struct CommandBufferVK {
    inline CommandBufferVK(DeviceVK& device) : m_Device(device) {
    }

    inline operator VkCommandBuffer() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~CommandBufferVK();

    void Create(VkCommandPool commandPool, VkCommandBuffer commandBuffer, CommandQueueType type);
    Result Create(const CommandBufferVKDesc& commandBufferDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    Result Begin(const DescriptorPool* descriptorPool);
    Result End();

    void SetPipeline(const Pipeline& pipeline);
    void SetPipelineLayout(const PipelineLayout& pipelineLayout);
    void SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets);
    void SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size);
    void SetDescriptorPool(const DescriptorPool& descriptorPool);
    void Barrier(const BarrierGroupDesc& barrierGroupDesc);

    void BeginRendering(const AttachmentsDesc& attachmentsDesc);
    void EndRendering();
    void SetViewports(const Viewport* viewports, uint32_t viewportNum);
    void SetScissors(const Rect* rects, uint32_t rectNum);
    void SetDepthBounds(float boundsMin, float boundsMax);
    void SetStencilReference(uint8_t frontRef, uint8_t backRef);
    void SetSamplePositions(const SamplePosition* positions, Sample_t positionNum, Sample_t sampleNum);
    void SetBlendConstants(const Color32f& color);
    void ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum);
    void SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType);
    void SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets);

    void Draw(const DrawDesc& drawDesc);
    void DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride);
    void DrawIndexed(const DrawIndexedDesc& drawIndexedDesc);
    void DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride);
    void Dispatch(const DispatchDesc& dispatchDesc);
    void DispatchIndirect(const Buffer& buffer, uint64_t offset);
    void BeginQuery(const QueryPool& queryPool, uint32_t offset);
    void EndQuery(const QueryPool& queryPool, uint32_t offset);
    void BeginAnnotation(const char* name);
    void EndAnnotation();

    void ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc);

    void ClearStorageTexture(const ClearStorageTextureDesc& clearDesc);

    void CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size);

    void CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc);

    void UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc);

    void ReadbackTextureToBuffer(Buffer& dstBuffer, TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc);

    void CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset);
    void ResetQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num);

    void BuildTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst,
        Buffer& scratch, uint64_t scratchOffset);

    void BuildBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst,
        Buffer& scratch, uint64_t scratchOffset);

    void UpdateTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst,
        AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset);

    void UpdateBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst,
        AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset);

    void CopyAccelerationStructure(AccelerationStructure& dst, AccelerationStructure& src, CopyMode copyMode);

    void WriteAccelerationStructureSize(
        const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset);

    void DispatchRays(const DispatchRaysDesc& dispatchRaysDesc);
    void DispatchRaysIndirect(const Buffer& buffer, uint64_t offset);

    void DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc);
    void DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride);

  private:
    void CopyWholeTexture(const TextureVK& dstTexture, const TextureVK& srcTexture);

  private:
    DeviceVK& m_Device;
    const PipelineVK* m_CurrentPipeline = nullptr;
    const PipelineLayoutVK* m_CurrentPipelineLayout = nullptr;
    VkCommandBuffer m_Handle = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    CommandQueueType m_Type = (CommandQueueType)0;
    VkPipelineBindPoint m_CurrentPipelineBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    VkPipelineLayout m_CurrentPipelineLayoutHandle = VK_NULL_HANDLE;
    Dim_t m_RenderLayerNum = 0;
    Dim_t m_RenderWidth = 0;
    Dim_t m_RenderHeight = 0;
};

} // namespace nri
