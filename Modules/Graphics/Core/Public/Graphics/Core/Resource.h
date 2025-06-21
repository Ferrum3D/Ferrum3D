#pragma once
#include <Graphics/Core/DeviceObject.h>
#include <festd/intrusive_list.h>

namespace FE::Graphics::Core
{
    //! @brief Resource usage flags.
    enum class ResourceUsage : uint32_t
    {
        //! @brief Specifies that the resource is only used on the GPU.
        kDeviceOnly,

        //! @brief Specifies that the resource can be mapped by the CPU and accessed in random order.
        kHostRandomAccess,

        //! @brief Specifies that the resource can be mapped and written through by the CPU, e.g. using memcpy.
        kHostWriteThrough,
    };


    enum class ResourceType : uint32_t
    {
        kUnknown,
        kBuffer,
        kTexture,
        kRenderTarget,
    };


    struct Resource
        : public DeviceObject
        , public festd::intrusive_list_node
    {
        [[nodiscard]] Env::Name GetName() const
        {
            return m_name;
        }

        [[nodiscard]] ResourceType GetType() const
        {
            return m_type;
        }

        [[nodiscard]] uint32_t GetResourceID() const
        {
            return m_resourceID;
        }

    protected:
        Env::Name m_name;
        uint32_t m_resourceID = kInvalidIndex;
        ResourceType m_type = ResourceType::kUnknown;

        void Register()
        {
            m_resourceID = m_device->RegisterResource(this);
        }

        ~Resource() override
        {
            m_device->UnregisterResource(m_resourceID, this);
            m_resourceID = kInvalidIndex;
        }
    };
} // namespace FE::Graphics::Core
