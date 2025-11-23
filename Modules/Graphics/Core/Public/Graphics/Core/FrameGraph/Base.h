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
