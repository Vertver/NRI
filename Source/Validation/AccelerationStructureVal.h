// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryVal;

struct AccelerationStructureVal final : public DeviceObjectVal<AccelerationStructure> {
    AccelerationStructureVal(DeviceVal& device, AccelerationStructure* accelerationStructure, bool isBoundToMemory) :
        DeviceObjectVal(device, accelerationStructure), m_IsBoundToMemory(isBoundToMemory) {
    }

    ~AccelerationStructureVal();

    inline bool IsBoundToMemory() const {
        return m_IsBoundToMemory;
    }

    inline void SetBoundToMemory(MemoryVal& memory) {
        m_Memory = &memory;
        m_IsBoundToMemory = true;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void GetMemoryInfo(MemoryDesc& memoryDesc) const;
    uint64_t GetUpdateScratchBufferSize() const;
    uint64_t GetBuildScratchBufferSize() const;
    uint64_t GetHandle() const;
    uint64_t GetNativeObject() const;
    Result CreateDescriptor(Descriptor*& descriptor);
    void SetDebugName(const char* name);

private:
    MemoryVal* m_Memory = nullptr;
    bool m_IsBoundToMemory = false;
};

} // namespace nri
