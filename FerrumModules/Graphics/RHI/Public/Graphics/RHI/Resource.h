#pragma once
#include <Graphics/RHI/DeviceObject.h>

namespace FE::Graphics::RHI
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


    struct Resource
        : public DeviceObject
        , public festd::intrusive_list_node
    {
        [[nodiscard]] Env::Name GetName() const
        {
            return m_name;
        }

    protected:
        Env::Name m_name;

        void Register()
        {
            std::lock_guard lk{ m_device->m_resourceListLock };
            m_device->m_resourceList.push_back(*this);
        }

        ~Resource() override
        {
            festd::intrusive_list<>::remove(*this);
        }
    };
} // namespace FE::Graphics::RHI
