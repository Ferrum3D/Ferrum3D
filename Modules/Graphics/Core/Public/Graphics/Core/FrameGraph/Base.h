#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Graphics::Core
{
    struct FrameGraph;
    struct FrameGraphBuilder;
    struct FrameGraphPassBuilder;
    struct FrameGraphBlackboard;
    struct FrameGraphContext;


    namespace Internal
    {
        constexpr uint32_t kFrameGraphResourceIndexBits = 16;
        constexpr uint32_t kFrameGraphResourceVersionBits = 10;

        template<class T>
        struct FrameGraphResourceHandle
        {
            struct Desc final
            {
                uint32_t m_resourceIndex : kFrameGraphResourceIndexBits;
                uint32_t m_version : kFrameGraphResourceVersionBits;
                uint32_t m_accessType : 6;
            };

            union
            {
                Desc m_desc;
                uint32_t m_value = kInvalidIndex;
            };

            [[nodiscard]] bool IsValid() const
            {
                return m_value != kInvalidIndex;
            }

            explicit operator uint32_t() const
            {
                return m_value;
            }

            explicit operator bool() const
            {
                return IsValid();
            }

            friend bool operator==(const FrameGraphResourceHandle lhs, const FrameGraphResourceHandle rhs)
            {
                return lhs.m_value == rhs.m_value;
            }

            friend bool operator!=(const FrameGraphResourceHandle lhs, const FrameGraphResourceHandle rhs)
            {
                return lhs.m_value != rhs.m_value;
            }

            static T Create(const uint32_t resourceIndex, const uint32_t resourceVersion, const uint32_t accessType)
            {
                T handle;
                handle.m_desc = { resourceIndex, resourceVersion, accessType };
                return handle;
            }

        private:
            FrameGraphResourceHandle() = default;

            friend T;
        };
    } // namespace Internal


    struct [[nodiscard]] RenderTargetHandle : public Internal::FrameGraphResourceHandle<RenderTargetHandle>
    {
        static const RenderTargetHandle kInvalid;
    };

    inline const RenderTargetHandle RenderTargetHandle::kInvalid = RenderTargetHandle{};


    struct [[nodiscard]] BufferHandle : public Internal::FrameGraphResourceHandle<BufferHandle>
    {
        static const BufferHandle kInvalid;
    };

    inline const BufferHandle BufferHandle::kInvalid = BufferHandle{};


    enum class ImageWriteType : uint32_t
    {
        kUndefined,
        kTransferDestination,
        kUnorderedAccess,
        kColorTarget,
        kDepthStencilTarget,
        kCount,
    };


    enum class ImageReadType : uint32_t
    {
        kTransferSource = festd::to_underlying(ImageWriteType::kCount),
        kShaderResource,
        kDepthRead,
        kCount,
    };


    enum class ImageAccessType : uint32_t
    {
        kUndefined,
        kTransferDestination,
        kUnorderedAccess,
        kColorTarget,
        kDepthStencilTarget,
        kTransferSource,
        kShaderResource,
        kDepthRead,
        kCount,
    };

    static_assert(festd::to_underlying(ImageAccessType::kCount) == festd::to_underlying(ImageReadType::kCount));


    enum class BufferWriteType : uint32_t
    {
        kUndefined,
        kTransferDestination,
        kUnorderedAccess,
        kCount,
    };


    enum class BufferReadType : uint32_t
    {
        kTransferSource = festd::to_underlying(BufferWriteType::kCount),
        kShaderResource,
        kIndirectArgument,
        kCount,
    };


    enum class BufferAccessType : uint32_t
    {
        kUndefined,
        kTransferDestination,
        kUnorderedAccess,
        kTransferSource,
        kShaderResource,
        kIndirectArgument,
        kCount,
    };

    static_assert(festd::to_underlying(BufferAccessType::kCount) == festd::to_underlying(BufferReadType::kCount));
} // namespace FE::Graphics::Core

namespace FE::Graphics
{
    struct [[nodiscard]] SamplerDescriptor final : public TypedHandle<SamplerDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] ImageSRVDescriptor final : public TypedHandle<ImageSRVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] ImageUAVDescriptor final : public TypedHandle<ImageUAVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] BufferSRVDescriptor final : public TypedHandle<BufferSRVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] BufferUAVDescriptor final : public TypedHandle<BufferUAVDescriptor, uint32_t>
    {
    };
} // namespace FE::Graphics
