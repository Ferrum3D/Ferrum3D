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
    inline void FeRTTI_Checks()                                                                                                  \
    {                                                                                                                            \
        using ThisType = std::remove_reference_t<decltype(*this)>;                                                               \
        static_assert(std::is_same_v<name, ThisType>);                                                                           \
    }                                                                                                                            \
                                                                                                                                 \
    inline static const ::FE::TypeID& FeRTTI_GetSID()                                                                            \
    {                                                                                                                            \
        static ::FE::TypeID id = ::FE::TypeID(uuid);                                                                             \
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
            return static_cast<TDstPtr>(src);

        return nullptr;
    }

    //! \brief Assert that a variable can be dynamically casted to another type.
    //!
    //! Works just like \ref fe_dynamic_cast<T*>, except it will assert that a type can be casted and won't return
    //! a `nullptr`. Use this when you're certainly sure that you can use `static_cast` here, but want to check it
    //! in debug builds. In release builds, when assertions are disabled, this can lead to undefined behaviour.
    //!
    //! \note The function uses only the classes that provide Ferrum3D RTTI through \ref FE_CLASS_RTTI.
    //!
    //! \tparam TDstPtr - Type of return value, e.g. `DerivedClass*`, _must_ be a pointer.
    //! \tparam TSrc    - Type of source value, e.g. `BaseClass`, _must not_ be a pointer.
    //! \param [in] src - The source value.
    //!
    //! \return The result pointer if destination type was derived from source type.
    template<class TDstPtr, class TSrc, std::enable_if_t<std::is_base_of_v<TSrc, std::remove_pointer_t<TDstPtr>>, bool> = true>
    inline TDstPtr fe_assert_cast(TSrc* src)
    {
        FE_CORE_ASSERT(src->template FeRTTI_Is<std::remove_pointer_t<TDstPtr>>(), "Assert cast failed");
        return static_cast<TDstPtr>(src);
    }

    //! \brief Get name of a type.
    //!
    //! This function returns a short name provided in \ref FE_CLASS_RTTI or \ref FE_STRUCT_RTTI.\n
    //! Example:
    //! \code{.cpp}
    //!     template<class T>
    //!     class Foo
    //!     {
    //!         FE_CLASS_RTTI(Foo, "4BDF1868-0E22-48CF-9DBA-8DD10F2A9D0C");
    //!         // members...
    //!     };
    //!
    //!     const char* fooName = fe_nameof<Foo<int>>(); // "Foo" - not "Foo<int>"!
    //! \endcode
    //!
    //! \note The provided type must implement Ferrum3D RTTI system through \ref FE_CLASS_RTTI or \ref FE_STRUCT_RTTI.
    //!
    //! \tparam T - Type to get name of.
    //!
    //! \return Short name of type T.
    template<class T>
    inline const char* fe_nameof() noexcept
    {
        return T::FeRTTI_GetSName();
    }

    //! \brief Get name of a type.
    //!
    //! This functions is same as \ref fe_nameof(), but can return name of derived class if called from a base class.\n
    //! Returns a short name provided in \ref FE_CLASS_RTTI or \ref FE_STRUCT_RTTI.\n
    //! Example:
    //! \code{.cpp}
    //!     class Base
    //!     {
    //!         FE_CLASS_RTTI(Base, "AB26B8C7-827F-4212-88B4-F71A5EFD6EEB");
    //!     };
    //!     class Derived : public Base
    //!     {
    //!         FE_CLASS_RTTI(Derived, "68CCD7DF-507F-4F3B-9EC3-001EEB33EB55");
    //!     };
    //!
    //!     const char* derivedName = fe_nameof(*static_cast<Base*>(new Derived)); // "Derived"
    //! \endcode
    //!
    //! For additional information see overload of \ref fe_nameof() without parameters.
    //!
    //! \note The provided type must implement Ferrum3D RTTI system throgh \ref FE_CLASS_RTTI or \ref FE_STRUCT_RTTI.
    //!
    //! \tparam T - Type to get name of.
    //!
    //! \return Short name of type T.
    //!
    //! \see fe_nameof()
    template<class T>
    inline const char* fe_nameof(const T& object)
    {
        return object.FeRTTI_GetName();
    }

    template<class T>
    inline const UUID& fe_typeid() noexcept
    {
        return T::FeRTTI_GetSID();
    }

    template<class T>
    inline const UUID& fe_typeid(const T& object)
    {
        return object.FeRTTI_GetID();
    }
} // namespace FE
