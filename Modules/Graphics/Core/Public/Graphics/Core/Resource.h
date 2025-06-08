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
        kImage,
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

    protected:
        Env::Name m_name;
        ResourceType m_type = ResourceType::kUnknown;

        void Register()
        {
            m_device->RegisterResource(this);
        }

        ~Resource() override
        {
            festd::intrusive_list<>::remove(*this);
        }
    };
} // namespace FE::Graphics::Core
