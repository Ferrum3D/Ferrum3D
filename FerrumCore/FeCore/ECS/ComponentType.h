#pragma once
#include <FeCore/Strings/StringSlice.h>

namespace FE::ECS
{
    //! \brief Describes a type of component.
    struct ComponentType
    {
        TypeID Type;      //!< ID of component's type.
        uint32_t Alignment; //!< Alignment of component data.
        uint32_t DataSize;  //!< Size of component data.

        FE_RTTI_Base(ComponentType, "D98BE686-B494-4C6A-82A2-D2EE6CDCEC2E");

        [[nodiscard]] inline uint32_t AlignedSize() const
        {
            return AlignUp(DataSize, Alignment);
        }

        //! \brief Create a ComponentType for a particular component.
        //!
        //! \tparam T - C++ type of the component. Must implement FE_RTTI_Base.
        //!
        //! \return The created ComponentType.
        template<class T>
        inline static ComponentType Create()
        {
            ComponentType result;
            result.Type = fe_typeid<T>();
            result.Alignment = static_cast<uint32_t>(alignof(T));
            result.DataSize = static_cast<uint32_t>(sizeof(T));
            return result;
        }

        inline friend bool operator==(const ComponentType& lhs, const ComponentType& rhs)
        {
            return lhs.Type == rhs.Type;
        }

        inline friend bool operator!=(const ComponentType& lhs, const ComponentType& rhs)
        {
            return !(rhs == lhs);
        }
    };
} // namespace FE::ECS

FE_MAKE_HASHABLE(FE::ECS::ComponentType, , value.Type);
