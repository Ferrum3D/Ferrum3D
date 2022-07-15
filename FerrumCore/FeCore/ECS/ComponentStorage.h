#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/SparseSet.h>
#include <FeCore/Containers/SparseStorage.h>
#include <FeCore/ECS/ComponentType.h>
#include <FeCore/ECS/ECSCommon.h>

namespace FE::ECS
{
    //! \brief Component storage descriptor.
    struct ComponentStorageDesc
    {
        ComponentType Type; //!< Type of component.

        inline ComponentStorageDesc() = default;

        inline explicit ComponentStorageDesc(const ComponentType& type)
            : Type(type)
        {
        }
    };

    //! \brief Component storage class. Stores memory pages with components of a single type.
    class ComponentStorage : public Object<IObject>
    {
        inline static constexpr USize FreeListCapacity = 8;

        inline static constexpr UInt32 ComponentCountPerPageLog2 = 8;
        inline static constexpr UInt32 ComponentCountPerPage     = 1 << ComponentCountPerPageLog2;
        inline static constexpr UInt32 ComponentCountPerPageMask = ComponentCountPerPage - 1;

        ComponentStorageDesc m_Desc;
        List<SparseStorage<UInt32>> m_Pages;

        // Cache some of removed values to optimize searching for free components.
        // We do not use the RLU here because we do not care when the component was last used, we just need storage.
        UInt32 m_FreeList[FreeListCapacity];
        UInt32 m_FreeListSize = 0;

    public:
        FE_STRUCT_RTTI(ComponentStorage, "010F0DA1-ABF1-4883-9274-2C67E48B99FB");

        inline ComponentStorage() = default;

        //! \brief Initialize a component storage and allocate the first memory page.
        void Init(const ComponentStorageDesc& desc);

        //! \brief Add an instance of component to the storage.
        //!
        //! \param [in] componentData - Component data to be copied.
        //! \param [out] id           - The ID of the created component within the storage.
        ECSResult AllocateComponent(void* componentData, UInt32& id);

        //! \brief Find a component in the storage and update its data.
        //!
        //! \param [in] componentData - Component data to be copied.
        //! \param [in] id            - The ID of the component to update.
        ECSResult UpdateComponent(void* componentData, UInt32 id);

        //! \brief Find a component in the storage and remove it.
        //!
        //! \param [in] id - The ID of the component to remove.
        ECSResult RemoveComponent(UInt32 id);

        //! \brief Add an instance of component to the storage.
        //!
        //! \tparam T - The type of the component, must be trivial!
        //!
        //! \param [in] component - Component data to be copied.
        //! \param [out] id       - The ID of the created component within the storage.
        template<class T>
        inline ECSResult AllocateComponent(const T& component, UInt32& id)
        {
            if (ValidationEnabled())
            {
                FE_ASSERT_MSG(fe_typeid<T>() == m_Desc.Type.Type,
                              "The type of component added to the storage must match "
                              "the type specified in ComponentStorage::Init() function");
            }

            return AllocateComponent(static_cast<void*>(&component), id);
        }

        //! \brief Find a component in the storage and update its data.
        //!
        //! \tparam T - The type of the component, must be trivial!
        //!
        //! \param [in] component - Component data to be copied.
        //! \param [in] id        - The ID of the component to update.
        template<class T>
        inline ECSResult UpdateComponent(const T& component, UInt32 id)
        {
            if (ValidationEnabled())
            {
                FE_ASSERT_MSG(fe_typeid<T>() == m_Desc.Type.Type,
                              "The type of component updated in the storage must match "
                              "the type specified in ComponentStorage::Init() function");
            }

            return UpdateComponent(static_cast<void*>(&component), id);
        }
    };
} // namespace FE::ECS
