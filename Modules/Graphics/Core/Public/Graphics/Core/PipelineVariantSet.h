#pragma once
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/ShaderVariant.h>

namespace FE::Graphics::Core
{
    struct PipelineVariantSetBase
    {
        virtual ~PipelineVariantSetBase() = default;

        PipelineVariantSetBase(const PipelineVariantSetBase&) = delete;
        PipelineVariantSetBase(PipelineVariantSetBase&&) = delete;
        PipelineVariantSetBase& operator=(const PipelineVariantSetBase&) = delete;
        PipelineVariantSetBase& operator=(PipelineVariantSetBase&&) = delete;

    protected:
        virtual void Compile(uint32_t variantIndex, PipelineFactory* factory) = 0;

        virtual Env::Name GetDefines(uint32_t variantIndex, std::pmr::memory_resource* tempAllocator) = 0;
        virtual festd::pmr::vector<ShaderSpecializationConstant> GetSpecializationConstants(
            uint32_t variantIndex, std::pmr::memory_resource* tempAllocator) = 0;

        [[nodiscard]] virtual bool IsVariantDiscarded([[maybe_unused]] const uint32_t variantIndex) const
        {
            return false;
        }

        PipelineVariantSetBase();

        festd::vector<PipelineBase*> m_pipelineVariants;
        PipelineVariantSetBase* m_next = nullptr;

    private:
        friend void CompileGlobalPipelineSets(PipelineFactory* factory);
        friend void WaitForGlobalPipelineSets();

        void Compile(PipelineFactory* factory);
    };


    void CompileGlobalPipelineSets(PipelineFactory* factory);
    void WaitForGlobalPipelineSets();


    struct GraphicsPipelineVariantSet : public PipelineVariantSetBase
    {
        using PipelineType = GraphicsPipeline;

    protected:
        virtual void SetupRequest(uint32_t variantIndex, GraphicsPipelineRequest& request) = 0;

        void Compile(uint32_t variantIndex, PipelineFactory* factory) final;
    };


    struct ComputePipelineVariantSet : public PipelineVariantSetBase
    {
        using PipelineType = ComputePipeline;

    protected:
        virtual void SetupRequest(uint32_t variantIndex, ComputePipelineRequest& request) = 0;

        void Compile(uint32_t variantIndex, PipelineFactory* factory) final;
    };


#define FE_DECLARE_PIPELINE_SET_2(ClassName, Specializer)                                                                        \
    ClassName()                                                                                                                  \
    {                                                                                                                            \
        m_pipelineVariants.resize(Specializer::kVariantCount);                                                                   \
    }                                                                                                                            \
                                                                                                                                 \
    FE::Env::Name GetDefines(const uint32_t variantIndex, std::pmr::memory_resource* tempAllocator) final                        \
    {                                                                                                                            \
        const Specializer specializer{ variantIndex };                                                                           \
        return specializer.GetDefines(tempAllocator);                                                                            \
    }                                                                                                                            \
                                                                                                                                 \
    FE::festd::pmr::vector<FE::Graphics::Core::ShaderSpecializationConstant> GetSpecializationConstants(                         \
        const uint32_t variantIndex, std::pmr::memory_resource* tempAllocator) final                                             \
    {                                                                                                                            \
        const Specializer specializer{ variantIndex };                                                                           \
        return specializer.GetSpecializationConstants(tempAllocator);                                                            \
    }                                                                                                                            \
                                                                                                                                 \
    [[nodiscard]] static PipelineType* GetPipeline(const Specializer& specializer)                                               \
    {                                                                                                                            \
        return static_cast<PipelineType*>(GInstance.m_pipelineVariants[specializer.GetVariantIndex()]);                          \
    }                                                                                                                            \
    static ClassName GInstance


#define FE_DECLARE_PIPELINE_SET_1(ClassName)                                                                                     \
    ClassName()                                                                                                                  \
    {                                                                                                                            \
        m_pipelineVariants.resize(1);                                                                                            \
    }                                                                                                                            \
                                                                                                                                 \
    FE::Env::Name GetDefines([[maybe_unused]] uint32_t variantIndex, [[maybe_unused]] std::pmr::memory_resource* tempAllocator)  \
        final                                                                                                                    \
    {                                                                                                                            \
        return Env::Name::kEmpty;                                                                                                \
    }                                                                                                                            \
                                                                                                                                 \
    FE::festd::pmr::vector<FE::Graphics::Core::ShaderSpecializationConstant> GetSpecializationConstants(                         \
        [[maybe_unused]] const uint32_t variantIndex, std::pmr::memory_resource* tempAllocator) final                            \
    {                                                                                                                            \
        return FE::festd::pmr::vector<FE::Graphics::Core::ShaderSpecializationConstant>{ tempAllocator };                        \
    }                                                                                                                            \
                                                                                                                                 \
    [[nodiscard]] static PipelineType* GetPipeline()                                                                             \
    {                                                                                                                            \
        return static_cast<PipelineType*>(GInstance.m_pipelineVariants[0]);                                                      \
    }                                                                                                                            \
    static ClassName GInstance


#define FE_DECLARE_PIPELINE_SET(...) FE_MACRO_SPECIALIZE(FE_DECLARE_PIPELINE_SET, __VA_ARGS__)


#define FE_IMPLEMENT_PIPELINE_SET(Name) Name Name::GInstance
} // namespace FE::Graphics::Core
