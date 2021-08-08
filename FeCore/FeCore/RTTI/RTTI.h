#pragma once
#include <FeCore/Utils/UUID.h>
#include <cstddef>
#include <cstdint>

namespace FE
{
    using TypeID = UUID;

    /**
     * @brief Same as `FE_CLASS_RTTI(name, uuid)`, but for final classes that don't inherit from other classes.
     * 
     * Use this whenever possiple, because `FE_CLASS_RTTI(name, uuid)` implements virtual functions.
    */
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

    /**
     * This macro is a part of Ferrum3D custom RTTI implementation.
     * Use it to allow `fe_dynamic_cast` on a class and usage of the class as a name of global variable in Environment.
     * Example:
     *     class Foo
     *     {
     *     public:
     *         FE_CLASS_RTTI(Foo, "12CED1D1-6337-443F-A854-B4624A6133AE");
     *         // members ...
     *     };
     * @note Use @ref FE_STRUCT_RTTI for `final` structs that don't inherit from any class.
    */
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

    template<class TDstPtr, class TSrc, std::enable_if_t<std::is_base_of_v<TSrc, std::remove_pointer_t<TDstPtr>>, bool> = true>
    inline TDstPtr fe_dynamic_cast(TSrc* src)
    {
        if (src->template FeRTTI_Is<std::remove_pointer_t<TDstPtr>>())
            return reinterpret_cast<TDstPtr>(src);
        return nullptr;
    }
} // namespace FE
