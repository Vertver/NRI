// © 2021 NVIDIA Corporation

#pragma once

// Redefine unused value to a type, we need
constexpr D3D12_QUERY_TYPE QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE = D3D12_QUERY_TYPE_VIDEO_DECODE_STATISTICS;

namespace nri {

struct DeviceD3D12;

struct QueryPoolD3D12 {
    inline QueryPoolD3D12(DeviceD3D12& device) : m_Device(device) {
    }

    inline ~QueryPoolD3D12() {
    }

    inline operator ID3D12QueryHeap*() const {
        return m_QueryHeap.GetInterface();
    }

    inline D3D12_QUERY_TYPE GetType() const {
        return m_QueryType;
    }

    inline ID3D12Resource* GetReadbackBuffer() const {
        return m_ReadbackBuffer.GetInterface();
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const QueryPoolDesc& queryPoolDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_QueryHeap, name);
    }

    inline uint32_t GetQuerySize() const {
        return m_QuerySize;
    }

private:
    Result CreateReadbackBuffer(const QueryPoolDesc& queryPoolDesc);

    DeviceD3D12& m_Device;
    D3D12_QUERY_TYPE m_QueryType = QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE;
    uint32_t m_QuerySize = 0;
    ComPtr<ID3D12QueryHeap> m_QueryHeap;
    ComPtr<ID3D12Resource> m_ReadbackBuffer;
};

} // namespace nri
