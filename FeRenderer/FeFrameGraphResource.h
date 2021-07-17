#pragma once
#include <FeCore/Utils/CoreUtils.h>
#include <cstdint>
#include <iostream>
#include <string>

namespace FE
{
    // clang-format off
    FE_ENUM(FeFrameGraphResourceType)
    {
        None,
        Texture,
        Buffer,
        Shader,
        RenderTarget
    };
    // clang-format on

    /**
     * @brief Handle to virtual mutable frame graph resource
     */
    struct FeFrameGraphMutResource
    {
        size_t Index{};
        std::string Name{};
        FeFrameGraphResourceType Type{};
        int Version{};
    };

    /**
     * @brief Handle to virtual immutable frame graph resource
     */
    struct FeFrameGraphResource
    {
        size_t Index{};
        std::string Name{};
        FeFrameGraphResourceType Type{};
        int Version{};

        inline FeFrameGraphResource() = default;

        inline FeFrameGraphResource(FeFrameGraphMutResource&& resource)
        {
            Index = resource.Index;
            Name  = resource.Name;
            Type  = resource.Type;
        }
    };

    inline std::ostream& operator<<(std::ostream& stream, const FeFrameGraphResource& resource)
    {
        return stream << resource.Name << '-' << resource.Version;
    }

    inline std::ostream& operator<<(std::ostream& stream, const FeFrameGraphMutResource& resource)
    {
        return stream << resource.Name << '-' << resource.Version;
    }
} // namespace FE
