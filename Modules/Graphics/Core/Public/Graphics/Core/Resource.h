#pragma once
#include <Graphics/Core/DeviceObject.h>
#include <festd/intrusive_list.h>

namespace FE::Graphics::Core
{
    struct ResourcePool;

    //! @brief Resource memory status.
    enum class ResourceMemory : uint32_t
    {
        //! @brief Specifies that the resource memory is not committed.
        kNotCommitted,

        //! @brief Specifies that the resource is only used on the GPU.
        kDeviceLocal,

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
    };


    template<class TResource, class TDesc, class TSubresource>
    struct BaseResourceView
    {
        TResource* m_resource = nullptr;
        TSubresource m_subresource = TSubresource::kInvalid;

        static const BaseResourceView kInvalid;

        BaseResourceView() = default;

        BaseResourceView(TResource* resource)
            : m_resource(resource)
        {
            if (resource)
                m_subresource = TSubresource::CreateWhole(resource->GetDesc());
        }

        BaseResourceView(TResource* resource, const TSubresource subresource)
            : m_resource(resource)
            , m_subresource(subresource)
        {
        }

        [[nodiscard]] const TDesc& GetBaseDesc() const
        {
            return m_resource->GetDesc();
        }

        [[nodiscard]] Env::Name GetName() const
        {
            return m_resource->GetName();
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_resource != nullptr;
        }
    };

    template<class TResource, class TDesc, class TSubresource>
    inline const BaseResourceView<TResource, TDesc, TSubresource> BaseResourceView<TResource, TDesc, TSubresource>::kInvalid = {
        nullptr,
        TSubresource::kInvalid
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

        [[nodiscard]] virtual ResourceMemory GetMemoryStatus() const = 0;

        virtual void DecommitMemory() = 0;

    protected:
        Env::Name m_name;
        uint32_t m_resourceID = kInvalidIndex;
        ResourceType m_type = ResourceType::kUnknown;

        void Register()
        {
            m_resourceID = m_device->RegisterResource(this);

            // Resources are never really destroyed immediately, they are only returned to the pool.
            // So, it's the pool's responsibility to destroy them when safe.
            SetImmediateDestroyPolicy();
        }

        ~Resource() override
        {
            m_device->UnregisterResource(m_resourceID, this);
            m_resourceID = kInvalidIndex;
        }
    };
} // namespace FE::Graphics::Core
