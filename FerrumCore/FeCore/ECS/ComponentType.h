#pragma once
#include <FeCore/Strings/StringSlice.h>

namespace FE::ECS
{
    //! \brief Describes a type of component.
    struct ComponentType
    {
        FE::StringSlice Name; //!< Name of component, must be a static string.
        TypeID Type;          //!< ID of component's type.
        USize Alignment;      //!< Alignment of component data.
        USize DataSize;       //!< Size of component data.

        FE_STRUCT_RTTI(ComponentType, "D98BE686-B494-4C6A-82A2-D2EE6CDCEC2E");

        [[nodiscard]] inline USize AlignedSize() const
        {
            return AlignUp(DataSize, Alignment);
        }

        //! \brief Create a ComponentType for a particular component.
        //!
        //! \tparam T - C++ type of the component. Must implement FE_STRUCT_RTTI.
        //!
        //! \return The created ComponentType.
        template<class T>
        inline static ComponentType Create()
        {
            ComponentType result;
            result.Name      = fe_nameof<T>();
            result.Type      = fe_typeid<T>();
            result.Alignment = alignof(T);
            result.DataSize  = sizeof(T);
            return result;
        }
    };
} // namespace FE::ECS

FE_MAKE_HASHABLE(FE::ECS::ComponentType, , value.Type);
