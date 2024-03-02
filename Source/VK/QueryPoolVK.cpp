// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "QueryPoolVK.h"

using namespace nri;

QueryPoolVK::~QueryPoolVK() {
    const auto& vk = m_Device.GetDispatchTable();

    if (m_OwnsNativeObjects)
        vk.DestroyQueryPool(m_Device, m_Handle, m_Device.GetAllocationCallbacks());
}

Result QueryPoolVK::Create(const QueryPoolDesc& queryPoolDesc) {
    m_OwnsNativeObjects = true;

    if (queryPoolDesc.queryType == QueryType::TIMESTAMP)
        m_Type = VK_QUERY_TYPE_TIMESTAMP;
    else if (queryPoolDesc.queryType == QueryType::OCCLUSION)
        m_Type = VK_QUERY_TYPE_OCCLUSION;
    else if (queryPoolDesc.queryType == QueryType::PIPELINE_STATISTICS)
        m_Type = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    else if (queryPoolDesc.queryType == QueryType::ACCELERATION_STRUCTURE_COMPACTED_SIZE)
        m_Type = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    else
        return Result::INVALID_ARGUMENT;

    VkQueryPipelineStatisticFlags pipelineStatistics =
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT | VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

    if (m_Device.GetDesc().isMeshShaderSupported)
        pipelineStatistics |= VK_QUERY_PIPELINE_STATISTIC_TASK_SHADER_INVOCATIONS_BIT_EXT | VK_QUERY_PIPELINE_STATISTIC_MESH_SHADER_INVOCATIONS_BIT_EXT;

    const VkQueryPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr, (VkQueryPoolCreateFlags)0, m_Type, queryPoolDesc.capacity, pipelineStatistics};

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateQueryPool(m_Device, &poolInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateQueryPool returned %d", (int32_t)result);

    m_QuerySize = (m_Type == VK_QUERY_TYPE_PIPELINE_STATISTICS ? (m_Device.GetDesc().isMeshShaderSupported ? 13 : 11) : 1) * sizeof(uint64_t);

    return Result::SUCCESS;
}

Result QueryPoolVK::Create(const QueryPoolVKDesc& queryPoolDesc) {
    m_OwnsNativeObjects = false;
    m_Type = (VkQueryType)queryPoolDesc.vkQueryType;
    m_Handle = (VkQueryPool)queryPoolDesc.vkQueryPool;
    m_QuerySize = (m_Type == VK_QUERY_TYPE_PIPELINE_STATISTICS ? (m_Device.GetDesc().isMeshShaderSupported ? 13 : 11) : 1) * sizeof(uint64_t);

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void QueryPoolVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_QUERY_POOL, (uint64_t)m_Handle, name);
}

#include "QueryPoolVK.hpp"