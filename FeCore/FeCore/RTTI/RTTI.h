#pragma once
#include <FeCore/Utils/UUID.h>
#include <cstddef>
#include <cstdint>

namespace FE
{
    using TypeID = UUID;

    //! \brief Define common RTTI functions for a struct.
    //!
    //! Same as \ref FE_CLASS_RTTI, but for `final` classes that don't inherit from other classes.\n
    //! Use this whenever possiple, because \ref FE_CLASS_RTTI implements virtual functions.
    //!
    //! \see FE_CLASS_RTTI
#define FE_STRUCT_RTTI(name, uuid)                                                                                               \
    inline static const ::FE::TypeID& FeRTTI_GetSID()                                                                            \
    {                                                                                                                            \
        static ::FE::TypeID id = uuid;                                                                                           \
        return id;                                                                                                               \
    }                                                                                                                            \
                                                                                                                                 \
    inline static const char* FeRTTI_GetSName()                                                                                  \
    {                                                                                                                            \
        return #name;                                                                                                            \
    }

    //! \brief Define common RTTI functions for a class.
    //!
    //! This macro is a part of Ferrum3D custom RTTI implementation.
    //! Use it to allow `fe_dynamic_cast` on a class and usage of the class as a name of global variable in Environment.\n
    //!
    //! Example:
    //! \code{.cpp}
    //!     class Foo
    //!     {
    //!     public:
    //!         FE_CLASS_RTTI(Foo, "12CED1D1-6337-443F-A854-B4624A6133AE");
    //!         // members ...
    //!     };
    //! \endcode
    //! 
    //! \note Use \ref FE_STRUCT_RTTI for `final` structs that don't inherit from any other class.
#define FE_CLASS_RTTI(name, uuid)                                                                                                \
    FE_STRUCT_RTTI(name, uuid);                                                                                                  \
    FE_PUSH_CLANG_WARNING("-Winconsistent-missing-override")                                                                     \
    inline virtual const char* FeRTTI_GetName() const                                                                            \
    {                                                                                                                            \
        return #name;                                                                                                            \
    }                                                                                                                            \
                                                                                                                                 \
    inline virtual ::FE::TypeID FeRTTI_GetID() const                                                                             \
    {                                                                                                                            \
        return name ::FeRTTI_GetSID();                                                                                           \
    }                                                                                                                            \
                                                                                                                                 \
    template<class FeRTTI_IS_TYPE>                                                                                               \
    inline bool FeRTTI_Is() const                                                                                                \
    {                                                                                                                            \
        return FeRTTI_IS_TYPE::FeRTTI_GetSID() == FeRTTI_GetID();                                                                \
    }                                                                                                                            \
    FE_POP_CLANG_WARNING

    //! \brief Cast a pointer to a base class to a derived class pointer if possible.
    //! 
    //! Works just like normal `dynamic_cast<T*>`, except it uses only the classes that provide Ferrum3D RTTI
    //! through \ref FE_CLASS_RTTI.
    //! 
    //! \tparam TDstPtr - Type of return value, e.g. `DerivedClass*`, _must_ be a pointer.
    //! \tparam TSrc    - Type of source value, e.g. `BaseClass`, _must not_ be a pointer.
    //! \param [in] src - The source value.
    //! 
    //! \return The result pointer if destination type was derived from source type, `nullptr` otherwise.
    template<class TDstPtr, class TSrc, std::enable_if_t<std::is_base_of_v<TSrc, std::remove_pointer_t<TDstPtr>>, bool> = true>
    inline TDstPtr fe_dynamic_cast(TSrc* src)
    {
        if (src->template FeRTTI_Is<std::remove_pointer_t<TDstPtr>>())
            return reinterpret_cast<TDstPtr>(src);
        return nullptr;
    }
} // namespace FE
