#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ArraySliceMut.h>
#include <FeCore/Containers/SparseSet.h>
#include <FeCore/Containers/SparseStorage.h>
#include <FeCore/ECS/ComponentType.h>
#include <FeCore/ECS/ECSCommon.h>

namespace FE::ECS
{
    //! \brief Component storage descriptor.
    struct ComponentStorageDesc
    {
        ComponentType Type;       //!< Type of the component.
        ArraySliceMut<Int8> Data; //!< Storage data.

        inline ComponentStorageDesc() = default;

        inline explicit ComponentStorageDesc(const ComponentType& type, const ArraySliceMut<Int8>& data)
            : Type(type)
            , Data(data)
        {
        }
    };

    //! \brief Component storage class. Stores view to components of a single type.
    class ComponentStorage final
    {
        friend class ArchetypeChunk;

        ComponentStorageDesc m_Desc;
        ArraySliceMut<Int8> m_Data;
        UInt32 m_Count = 0;

        inline void AllocateComponentUnchecked()
        {
            ++m_Count;
        }

    public:
        FE_STRUCT_RTTI(ComponentStorage, "010F0DA1-ABF1-4883-9274-2C67E48B99FB");

        inline ComponentStorage() = default;

        //! \brief Initialize a component storage and allocate the first memory page.
        void Init(const ComponentStorageDesc& desc);

        //! \brief Size of the stored component aligned to the component type alignment.
        [[nodiscard]] inline USize ElementSize() const
        {
            return m_Desc.Type.AlignedSize();
        }

        //! \brief Get number of stored components.
        [[nodiscard]] inline USize Count() const
        {
            return m_Count;
        }

        [[nodiscard]] inline bool CheckTypeID(const TypeID& typeID) const
        {
            return m_Desc.Type.Type == typeID;
        }

        //! \brief Add an instance of component to the storage.
        //!
        //! \param [out] id  - The ID of the created component within the storage.
        bool AllocateComponentImpl(UInt32& id);

        //! \brief Find a component in the storage and update its data.
        //!
        //! \param [in] componentData - Component data to be copied.
        //! \param [in] id            - The ID of the component to update.
        void UpdateComponentImpl(const void* componentData, UInt32 id);

        //! \brief Find a component in the storage retrieve a pointer to its data.
        //!
        //! \param [in] id             - The ID of the component.
        //! \param [out] componentData - Component data.
        void ComponentData(UInt32 id, void** componentData);

        //! \brief Find a component in the storage and remove it.
        //!
        //! \param [in] id - The ID of the component to remove.
        //!
        //! \return Index of the component that was moved to the place of removed one or -1.
        Int32 RemoveComponent(UInt32 id);

        template<class T>
        inline T* GetComponent(UInt32 id)
        {
            void* result;
            ComponentData(id, &result);
            return static_cast<T*>(result);
        }

        //! \brief Add an instance of component to the storage.
        //!
        //! \tparam T - The type of the component, must be trivial!
        //!
        //! \param [in] component - Component data to be copied.
        //! \param [out] id       - The ID of the created component within the storage.
        //!
        //! \return True on success.
        template<class T>
        inline bool AllocateComponent(const T& component, UInt32& id)
        {
            if (ValidationEnabled())
            {
                FE_ASSERT_MSG(fe_typeid<T>() == m_Desc.Type.Type,
                              "The type of component added to the storage must match "
                              "the type specified in ComponentStorage::Init() function");
            }

            if (AllocateComponentImpl(id))
            {
                UpdateComponentImpl(static_cast<const void*>(&component), id);
                return true;
            }

            return false;
        }

        //! \brief Find a component in the storage and update its data.
        //!
        //! \tparam T - The type of the component, must be trivial!
        //!
        //! \param [in] component - Component data to be copied.
        //! \param [in] id        - The ID of the component to update.
        template<class T>
        inline void UpdateComponent(const T& component, UInt32 id)
        {
            if (ValidationEnabled())
            {
                FE_ASSERT_MSG(fe_typeid<T>() == m_Desc.Type.Type,
                              "The type of component updated in the storage must match "
                              "the type specified in ComponentStorage::Init() function");
            }

            UpdateComponentImpl(static_cast<const void*>(&component), id);
        }
    };
} // namespace FE::ECS
