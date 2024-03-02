// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

#include "PipelineD3D11.h"

using namespace nri;

Result PipelineD3D11::Create(const GraphicsPipelineDesc& pipelineDesc) {
    const ShaderDesc* vertexShader = nullptr;
    HRESULT hr;

    m_PipelineLayout = (const PipelineLayoutD3D11*)pipelineDesc.pipelineLayout;

    { // Shaders
        for (uint32_t i = 0; i < pipelineDesc.shaderNum; i++) {
            const ShaderDesc* shaderDesc = pipelineDesc.shaders + i;

            if (shaderDesc->stage == StageBits::VERTEX_SHADER) {
                vertexShader = shaderDesc;
                hr = m_Device->CreateVertexShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_VertexShader);
                RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateVertexShader()");
            } else if (shaderDesc->stage == StageBits::TESS_CONTROL_SHADER) {
                hr = m_Device->CreateHullShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_TessControlShader);
                RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateHullShader()");
            } else if (shaderDesc->stage == StageBits::TESS_EVALUATION_SHADER) {
                hr = m_Device->CreateDomainShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_TessEvaluationShader);
                RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateDomainShader()");
            } else if (shaderDesc->stage == StageBits::GEOMETRY_SHADER) {
                hr = m_Device->CreateGeometryShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_GeometryShader);
                RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateGeometryShader()");
            } else if (shaderDesc->stage == StageBits::FRAGMENT_SHADER) {
                hr = m_Device->CreatePixelShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_FragmentShader);
                RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreatePixelShader()");
            } else
                return Result::INVALID_ARGUMENT;
        }
    }

    { // Input assembly
        const InputAssemblyDesc& ia = pipelineDesc.inputAssembly;

        m_Topology = GetD3D11TopologyFromTopology(ia.topology, ia.tessControlPointNum);
    }

    // Vertex input
    if (pipelineDesc.vertexInput) {
        const VertexInputDesc& vi = *pipelineDesc.vertexInput;

        uint32_t maxBindingSlot = 0;
        for (uint32_t i = 0; i < vi.streamNum; i++) {
            const VertexStreamDesc& stream = vi.streams[i];
            if (stream.bindingSlot > maxBindingSlot)
                maxBindingSlot = stream.bindingSlot;
        }
        m_InputAssemplyStrides.resize(maxBindingSlot + 1);

        D3D11_INPUT_ELEMENT_DESC* inputElements = STACK_ALLOC(D3D11_INPUT_ELEMENT_DESC, vi.attributeNum);
        for (uint32_t i = 0; i < vi.attributeNum; i++) {
            const VertexAttributeDesc& attrIn = vi.attributes[i];
            const VertexStreamDesc& stream = vi.streams[attrIn.streamIndex];
            D3D11_INPUT_ELEMENT_DESC& attrOut = inputElements[i];
            const DxgiFormat& dxgiFormat = GetDxgiFormat(attrIn.format);

            attrOut.SemanticName = attrIn.d3d.semanticName;
            attrOut.SemanticIndex = attrIn.d3d.semanticIndex;
            attrOut.Format = dxgiFormat.typed;
            attrOut.InputSlot = stream.bindingSlot;
            attrOut.AlignedByteOffset = attrIn.offset;
            attrOut.InstanceDataStepRate = stream.stepRate == VertexStreamStepRate::PER_VERTEX ? 0 : 1;
            attrOut.InputSlotClass = stream.stepRate == VertexStreamStepRate::PER_VERTEX ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA;

            m_InputAssemplyStrides[stream.bindingSlot] = stream.stride;
        };

        assert(vertexShader != nullptr);
        hr = m_Device->CreateInputLayout(&inputElements[0], vi.attributeNum, vertexShader->bytecode, (size_t)vertexShader->size, &m_InputLayout);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateInputLayout()");
    }

    // Multisample
    Sample_t sampleNum = 1;
    if (pipelineDesc.multisample) {
        if (pipelineDesc.multisample->sampleMask != ALL_SAMPLES)
            m_SampleMask = pipelineDesc.multisample->sampleMask;

        sampleNum = pipelineDesc.multisample->sampleNum;
    }

    { // Rasterization
        const RasterizationDesc& r = pipelineDesc.rasterization;

        D3D11_RASTERIZER_DESC2 rasterizerDesc = {};
        // D3D11_RASTERIZER_DESC
        rasterizerDesc.FillMode = r.fillMode == FillMode::SOLID ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
        rasterizerDesc.CullMode = GetD3D11CullModeFromCullMode(r.cullMode);
        rasterizerDesc.FrontCounterClockwise = r.frontCounterClockwise;
        rasterizerDesc.DepthBias = (INT)r.depthBias;
        rasterizerDesc.DepthBiasClamp = r.depthBiasClamp;
        rasterizerDesc.SlopeScaledDepthBias = r.depthBiasSlopeFactor;
        rasterizerDesc.DepthClipEnable = r.depthClamp;
        rasterizerDesc.ScissorEnable = TRUE;
        rasterizerDesc.AntialiasedLineEnable = r.antialiasedLines;
        rasterizerDesc.MultisampleEnable = sampleNum > 1 ? TRUE : FALSE;
        // D3D11_RASTERIZER_DESC1
        rasterizerDesc.ForcedSampleCount = sampleNum;
        // D3D11_RASTERIZER_DESC2
        rasterizerDesc.ConservativeRaster = r.conservativeRasterization ? D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        RasterizerState rasterizerState = {};
        if (m_Device.GetVersion() >= 3) {
            hr = m_Device->CreateRasterizerState2(&rasterizerDesc, &rasterizerState.ptr);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device3::CreateRasterizerState2()");
        } else if (m_Device.GetVersion() >= 1) {
            hr = m_Device->CreateRasterizerState1((D3D11_RASTERIZER_DESC1*)&rasterizerDesc, (ID3D11RasterizerState1**)&rasterizerState.ptr);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device3::CreateRasterizerState1()");
        } else {
            hr = m_Device->CreateRasterizerState((D3D11_RASTERIZER_DESC*)&rasterizerDesc, (ID3D11RasterizerState**)&rasterizerState.ptr);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateRasterizerState()");
        }
        m_RasterizerStates.push_back(rasterizerState);

        // Ex
        memcpy(&m_RasterizerStateExDesc, &rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
        m_RasterizerStateExDesc.ForcedSampleCount = sampleNum;
        m_RasterizerStateExDesc.ProgrammableSamplePositionsEnable = true;
        m_RasterizerStateExDesc.SampleCount = sampleNum;
        m_RasterizerStateExDesc.ConservativeRasterEnable = r.conservativeRasterization;
        m_RasterizerStateExDesc.TargetIndepentRasterWithDepth = true;
    }

    { // Depth-stencil
        const DepthAttachmentDesc& da = pipelineDesc.outputMerger.depth;
        const StencilAttachmentDesc& sa = pipelineDesc.outputMerger.stencil;

        D3D11_DEPTH_STENCIL_DESC depthStencilState = {};
        depthStencilState.DepthEnable = da.compareFunc == CompareFunc::NONE ? FALSE : TRUE;
        depthStencilState.DepthWriteMask = da.write ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        depthStencilState.DepthFunc = GetD3D11ComparisonFuncFromCompareFunc(da.compareFunc);
        depthStencilState.StencilEnable = (sa.front.compareFunc == CompareFunc::NONE && sa.back.compareFunc == CompareFunc::NONE) ? FALSE : TRUE;
        depthStencilState.StencilReadMask = sa.front.compareMask;
        depthStencilState.StencilWriteMask = sa.front.writeMask;

        depthStencilState.FrontFace.StencilFailOp = GetD3D11StencilOpFromStencilFunc(sa.front.fail);
        depthStencilState.FrontFace.StencilDepthFailOp = GetD3D11StencilOpFromStencilFunc(sa.front.depthFail);
        depthStencilState.FrontFace.StencilPassOp = GetD3D11StencilOpFromStencilFunc(sa.front.pass);
        depthStencilState.FrontFace.StencilFunc = GetD3D11ComparisonFuncFromCompareFunc(sa.front.compareFunc);

        depthStencilState.BackFace.StencilFailOp = GetD3D11StencilOpFromStencilFunc(sa.front.fail);
        depthStencilState.BackFace.StencilDepthFailOp = GetD3D11StencilOpFromStencilFunc(sa.front.depthFail);
        depthStencilState.BackFace.StencilPassOp = GetD3D11StencilOpFromStencilFunc(sa.front.pass);
        depthStencilState.BackFace.StencilFunc = GetD3D11ComparisonFuncFromCompareFunc(sa.back.compareFunc);

        hr = m_Device->CreateDepthStencilState(&depthStencilState, &m_DepthStencilState);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateDepthStencilState()");
    }

    { // Blending
        const OutputMergerDesc& om = pipelineDesc.outputMerger;

        D3D11_BLEND_DESC1 blendState1 = {};
        blendState1.AlphaToCoverageEnable = (pipelineDesc.multisample && pipelineDesc.multisample->alphaToCoverage) ? TRUE : FALSE;
        blendState1.IndependentBlendEnable = TRUE;
        for (uint32_t i = 0; i < om.colorNum; i++) {
            const ColorAttachmentDesc& bs = om.color[i];
            blendState1.RenderTarget[i].BlendEnable = bs.blendEnabled;
            blendState1.RenderTarget[i].SrcBlend = GetD3D11BlendFromBlendFactor(bs.colorBlend.srcFactor);
            blendState1.RenderTarget[i].DestBlend = GetD3D11BlendFromBlendFactor(bs.colorBlend.dstFactor);
            blendState1.RenderTarget[i].BlendOp = GetD3D11BlendOpFromBlendFunc(bs.colorBlend.func);
            blendState1.RenderTarget[i].SrcBlendAlpha = GetD3D11BlendFromBlendFactor(bs.alphaBlend.srcFactor);
            blendState1.RenderTarget[i].DestBlendAlpha = GetD3D11BlendFromBlendFactor(bs.alphaBlend.dstFactor);
            blendState1.RenderTarget[i].BlendOpAlpha = GetD3D11BlendOpFromBlendFunc(bs.alphaBlend.func);
            blendState1.RenderTarget[i].RenderTargetWriteMask = uint8_t(bs.colorWriteMask);
            blendState1.RenderTarget[i].LogicOpEnable = om.colorLogicFunc == LogicFunc::NONE ? FALSE : TRUE;
            blendState1.RenderTarget[i].LogicOp = GetD3D11LogicOpFromLogicFunc(om.colorLogicFunc);
        }

        if (m_Device.GetVersion() >= 1)
            hr = m_Device->CreateBlendState1(&blendState1, &m_BlendState);
        else {
            D3D11_BLEND_DESC blendState = {};
            blendState.AlphaToCoverageEnable = blendState1.AlphaToCoverageEnable;
            blendState.IndependentBlendEnable = blendState1.IndependentBlendEnable;
            for (uint32_t i = 0; i < om.colorNum; i++) {
                blendState.RenderTarget[i].BlendEnable = blendState1.RenderTarget[i].BlendEnable;
                blendState.RenderTarget[i].SrcBlend = blendState1.RenderTarget[i].SrcBlend;
                blendState.RenderTarget[i].DestBlend = blendState1.RenderTarget[i].DestBlend;
                blendState.RenderTarget[i].BlendOp = blendState1.RenderTarget[i].BlendOp;
                blendState.RenderTarget[i].SrcBlendAlpha = blendState1.RenderTarget[i].SrcBlendAlpha;
                blendState.RenderTarget[i].DestBlendAlpha = blendState1.RenderTarget[i].DestBlendAlpha;
                blendState.RenderTarget[i].BlendOpAlpha = blendState1.RenderTarget[i].BlendOpAlpha;
                blendState.RenderTarget[i].RenderTargetWriteMask = blendState1.RenderTarget[i].RenderTargetWriteMask;
            }

            hr = m_Device->CreateBlendState(&blendState, (ID3D11BlendState**)&m_BlendState);
        }

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device1::CreateBlendState1()");
    }

    return Result::SUCCESS;
}

Result PipelineD3D11::Create(const ComputePipelineDesc& pipelineDesc) {
    if (pipelineDesc.shader.bytecode) {
        HRESULT hr = m_Device->CreateComputeShader(pipelineDesc.shader.bytecode, (size_t)pipelineDesc.shader.size, nullptr, &m_ComputeShader);

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateComputeShader()");
    }

    m_PipelineLayout = (const PipelineLayoutD3D11*)pipelineDesc.pipelineLayout;

    return Result::SUCCESS;
}

void PipelineD3D11::ChangeSamplePositions(ID3D11DeviceContextBest* deferredContext, const SamplePositionsState& samplePositionState) {
    if (IsCompute())
        return;

    // Find in cached states
    size_t i = 0;
    for (; i < m_RasterizerStates.size(); i++) {
        if (m_RasterizerStates[i].samplePositionHash == samplePositionState.positionHash)
            break;
    }

    // Add a new state, if not found
    if (i == m_RasterizerStates.size()) {
        RasterizerState newState = {};
        newState.samplePositionHash = samplePositionState.positionHash;

        m_RasterizerStateExDesc.InterleavedSamplingEnable = samplePositionState.positionNum > m_RasterizerStateExDesc.SampleCount;
        for (uint32_t j = 0; j < samplePositionState.positionNum; j++) {
            m_RasterizerStateExDesc.SamplePositionsX[j] = samplePositionState.positions[j].x + 8;
            m_RasterizerStateExDesc.SamplePositionsY[j] = samplePositionState.positions[j].y + 8;
        }

        if (m_Device.GetExt()->HasNVAPI()) {
            NvAPI_Status result = NvAPI_D3D11_CreateRasterizerState(m_Device.GetNativeObject(), &m_RasterizerStateExDesc, (ID3D11RasterizerState**)&newState.ptr);
            if (result != NVAPI_OK)
                REPORT_ERROR(&m_Device, "NvAPI_D3D11_CreateRasterizerState() - FAILED!");
        }

        if (newState.ptr)
            m_RasterizerStates.push_back(newState);
        else
            i = 0;
    }

    // Bind
    ID3D11RasterizerState2* stateWithPSL = m_RasterizerStates[i].ptr;
    deferredContext->RSSetState(stateWithPSL);
}

void PipelineD3D11::ChangeStencilReference(ID3D11DeviceContextBest* deferredContext, uint8_t stencilRef) {
    if (IsCompute())
        return;

    deferredContext->OMSetDepthStencilState(m_DepthStencilState, stencilRef);
}

void PipelineD3D11::ChangeBlendConstants(ID3D11DeviceContextBest* deferredContext, const Color32f& color) {
    if (IsCompute())
        return;

    deferredContext->OMSetBlendState(m_BlendState, &color.x, m_SampleMask);
}

void PipelineD3D11::Bind(ID3D11DeviceContextBest* deferredContext, const PipelineD3D11* currentPipeline, uint8_t stencilRef, const Color32f& blendFactor,
    const SamplePositionsState& samplePositionState) {
    if (IsCompute()) {
        if (!currentPipeline || m_ComputeShader != currentPipeline->m_ComputeShader)
            deferredContext->CSSetShader(m_ComputeShader, nullptr, 0);
    } else {
        if (!currentPipeline || m_Topology != currentPipeline->m_Topology)
            deferredContext->IASetPrimitiveTopology(m_Topology);

        if (!currentPipeline || m_InputLayout != currentPipeline->m_InputLayout)
            deferredContext->IASetInputLayout(m_InputLayout);

        if (!currentPipeline || m_VertexShader != currentPipeline->m_VertexShader)
            deferredContext->VSSetShader(m_VertexShader, nullptr, 0);

        if (!currentPipeline || m_TessControlShader != currentPipeline->m_TessControlShader)
            deferredContext->HSSetShader(m_TessControlShader, nullptr, 0);

        if (!currentPipeline || m_TessEvaluationShader != currentPipeline->m_TessEvaluationShader)
            deferredContext->DSSetShader(m_TessEvaluationShader, nullptr, 0);

        if (!currentPipeline || m_GeometryShader != currentPipeline->m_GeometryShader)
            deferredContext->GSSetShader(m_GeometryShader, nullptr, 0);

        if (!currentPipeline || m_FragmentShader != currentPipeline->m_FragmentShader)
            deferredContext->PSSetShader(m_FragmentShader, nullptr, 0);

        // Dynamic state // TODO: uncached
        if (!m_RasterizerStates.empty())
            ChangeSamplePositions(deferredContext, samplePositionState);

        if (m_DepthStencilState)
            deferredContext->OMSetDepthStencilState(m_DepthStencilState, stencilRef);

        if (m_BlendState)
            deferredContext->OMSetBlendState(m_BlendState, &blendFactor.x, m_SampleMask);
    }
}

//================================================================================================================
// NRI
//================================================================================================================

void PipelineD3D11::SetDebugName(const char* name) {
    SET_D3D_DEBUG_OBJECT_NAME(m_VertexShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_TessControlShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_TessEvaluationShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_GeometryShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_FragmentShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_ComputeShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_InputLayout, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_DepthStencilState, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_BlendState, name);

    for (size_t i = 0; i < m_RasterizerStates.size(); i++)
        SET_D3D_DEBUG_OBJECT_NAME(m_RasterizerStates[i].ptr, name);
}
