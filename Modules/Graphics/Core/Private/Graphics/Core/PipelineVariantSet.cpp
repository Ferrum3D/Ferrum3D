#include <FeCore/Memory/FiberTempAllocator.h>
#include <Graphics/Core/PipelineVariantSet.h>

namespace FE::Graphics::Core
{
    namespace
    {
        PipelineVariantSetBase* GPipelineSetList = nullptr;
    }


    PipelineVariantSetBase::PipelineVariantSetBase()
    {
        m_next = GPipelineSetList;
        GPipelineSetList = this;
    }


    void PipelineVariantSetBase::Compile(PipelineFactory* factory)
    {
        for (uint32_t pipelineIndex = 0; pipelineIndex < m_pipelineVariants.size(); ++pipelineIndex)
        {
            if (!IsVariantDiscarded(pipelineIndex))
                Compile(pipelineIndex, factory);
        }
    }


    void CompileGlobalPipelineSets(PipelineFactory* factory)
    {
        PipelineVariantSetBase* pipelineSet = GPipelineSetList;
        while (pipelineSet)
        {
            pipelineSet->Compile(factory);
            pipelineSet = pipelineSet->m_next;
        }
    }


    void WaitForGlobalPipelineSets()
    {
        PipelineVariantSetBase* pipelineSet = GPipelineSetList;
        while (pipelineSet)
        {
            for (const PipelineBase* pipeline : pipelineSet->m_pipelineVariants)
            {
                if (pipeline)
                    pipeline->GetCompletionWaitGroup()->Wait();
            }

            pipelineSet = pipelineSet->m_next;
        }
    }


    void GraphicsPipelineVariantSet::Compile(const uint32_t variantIndex, PipelineFactory* factory)
    {
        Memory::FiberTempAllocator temp;
        GraphicsPipelineRequest request;

        const auto specializationConstants = GetSpecializationConstants(variantIndex, &temp);
        request.m_defines = GetDefines(variantIndex, &temp);
        request.m_specializationConstants = specializationConstants;

        SetupRequest(variantIndex, request);
        m_pipelineVariants[variantIndex] = factory->CreateGraphicsPipeline(request);
    }


    void ComputePipelineVariantSet::Compile(const uint32_t variantIndex, PipelineFactory* factory)
    {
        Memory::FiberTempAllocator temp;
        ComputePipelineRequest request;

        const auto specializationConstants = GetSpecializationConstants(variantIndex, &temp);
        request.m_defines = GetDefines(variantIndex, &temp);
        request.m_specializationConstants = specializationConstants;

        SetupRequest(variantIndex, request);
        m_pipelineVariants[variantIndex] = factory->CreateComputePipeline(request);
    }
} // namespace FE::Graphics::Core
