#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Graphics
{
    struct [[nodiscard]] SamplerDescriptor final : public TypedHandle<SamplerDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] TextureSRVDescriptor final : public TypedHandle<TextureSRVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] TextureUAVDescriptor final : public TypedHandle<TextureUAVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] BufferSRVDescriptor final : public TypedHandle<BufferSRVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] BufferUAVDescriptor final : public TypedHandle<BufferUAVDescriptor, uint32_t>
    {
    };
} // namespace FE::Graphics

FE_RTTI_Reflect(FE::Graphics::TextureSRVDescriptor, "4B5172EF-1E7A-43E2-B589-1BAC65428E92");
FE_RTTI_Reflect(FE::Graphics::TextureUAVDescriptor, "39287D51-E573-4428-AADF-74827CD4351E");
FE_RTTI_Reflect(FE::Graphics::BufferSRVDescriptor, "9FFDEB45-4643-43D9-859B-895C5E8012CF");
FE_RTTI_Reflect(FE::Graphics::BufferUAVDescriptor, "B03004C4-3108-46B1-A475-631CDC59BEDD");
FE_RTTI_Reflect(FE::Graphics::SamplerDescriptor, "0ADC65DE-1BA6-4A60-B320-333AD0C2F27B");


namespace FE::Graphics::Core
{
    struct FrameGraph;
    struct FrameGraphPassBuilder;
    struct FrameGraphBlackboard;
    struct FrameGraphContext;


    enum class DescriptorType : uint32_t
    {
        kInvalid,
        kSampler,
        kSRV,
        kUAV,
    };


    struct [[nodiscard]] FrameGraphTextureDescriptorHandle final
    {
        operator TextureSRVDescriptor() const;
        operator TextureUAVDescriptor() const;

    private:
        friend FrameGraph;

        FrameGraph* m_graph = nullptr;
        uint32_t m_descriptorIndex = kInvalidIndex;
    };


    struct [[nodiscard]] FrameGraphBufferDescriptorHandle final
    {
        operator BufferSRVDescriptor() const;
        operator BufferUAVDescriptor() const;

    private:
        friend FrameGraph;

        FrameGraph* m_graph = nullptr;
        uint32_t m_descriptorIndex = kInvalidIndex;
    };
} // namespace FE::Graphics::Core
