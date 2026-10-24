#pragma once
#include <Core/RTTI/Reflection.h>

namespace FE::Rtti
{
    //! @brief Customization point that describes how traversal and editing see a C++ type.
    //!
    //! A specialization provides `static void Bind(Type& type)`, which sets the type's shape and may add type attributes.
    //! It applies to reflected and synthesized types alike and replaces the default shape (kStruct for reflected classes).
    //! Specializations must be visible wherever fields of the type are reflected, next to its Serializer.
    template<class T>
    struct Shape
    {
    };


    template<class T>
    concept HasShape = requires(Type& type) { Shape<T>::Bind(type); };


    template<class T>
    const Type& ResolveType();
} // namespace FE::Rtti
