// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct DescriptorSetDesc;

struct DescriptorSetVK {
    inline DescriptorSetVK(DeviceVK& device) : m_Device(device) {
    }

    inline VkDescriptorSet GetHandle() const {
        return m_Handle;
    }

    inline uint32_t GetDynamicConstantBufferNum() const {
        return m_DynamicConstantBufferNum;
    }

    void Create(VkDescriptorSet handle, const DescriptorSetDesc& setDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    void UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs);
    void UpdateDynamicConstantBuffers(uint32_t bufferOffset, uint32_t descriptorNum, const Descriptor* const* descriptors);
    void Copy(const DescriptorSetCopyDesc& descriptorSetCopyDesc);

private:
    DeviceVK& m_Device;
    VkDescriptorSet m_Handle = VK_NULL_HANDLE;
    const DescriptorSetDesc* m_Desc = nullptr;
    uint32_t m_DynamicConstantBufferNum = 0;
};

} // namespace nri