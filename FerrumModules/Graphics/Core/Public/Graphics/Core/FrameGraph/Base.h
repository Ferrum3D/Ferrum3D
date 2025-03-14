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

            friend bool operator==(FrameGraphResourceHandle lhs, FrameGraphResourceHandle rhs)
            {
                return lhs.m_value == rhs.m_value;
            }

            friend bool operator!=(FrameGraphResourceHandle lhs, FrameGraphResourceHandle rhs)
            {
                return lhs.m_value != rhs.m_value;
            }

            static T Create(uint32_t resourceIndex, uint32_t resourceVersion)
            {
                T handle;
                handle.m_desc = { resourceIndex, resourceVersion };
                return handle;
            }

        private:
            FrameGraphResourceHandle() = default;

            friend T;
        };
    } // namespace Internal


    struct [[nodiscard]] ImageHandle : public Internal::FrameGraphResourceHandle<ImageHandle>
    {
        static const ImageHandle kInvalid;
    };

    inline const ImageHandle ImageHandle::kInvalid = ImageHandle{};


    struct [[nodiscard]] BufferHandle : public Internal::FrameGraphResourceHandle<BufferHandle>
    {
        static const BufferHandle kInvalid;
    };

    inline const BufferHandle BufferHandle::kInvalid = BufferHandle{};


    enum class ImageWriteType : uint32_t
    {
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


    enum class BufferWriteType : uint32_t
    {
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
} // namespace FE::Graphics::Core
