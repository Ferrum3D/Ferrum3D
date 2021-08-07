#pragma once
#include <FeCore/Utils/UUID.h>
#include <cstddef>
#include <cstdint>

namespace FE
{
    using TypeID = UUID;

#define FE_RTTI(name, uuid)                                                                                                      \
    inline virtual const char* const FeRTTI_GetName() const                                                                      \
    {                                                                                                                            \
        return #name;                                                                                                            \
    }                                                                                                                            \
                                                                                                                                 \
    inline virtual ::FE::TypeID FeRTTI_GetID() const                                                                             \
    {                                                                                                                            \
        return name ::FeRTTI_GetSID();                                                                                           \
    }                                                                                                                            \
                                                                                                                                 \
    inline static ::FE::TypeID FeRTTI_GetSID()                                                                                   \
    {                                                                                                                            \
        static ::FE::TypeID id = uuid;                                                                                           \
        return id;                                                                                                               \
    }                                                                                                                            \
                                                                                                                                 \
    inline static const char* FeRTTI_GetSName()                                                                                  \
    {                                                                                                                            \
        return #name;                                                                                                            \
    }                                                                                                                            \
                                                                                                                                 \
    template<class T>                                                                                                            \
    inline bool FeRTTI_Is() const                                                                                                \
    {                                                                                                                            \
        return T::FeRTTI_GetSID() == FeRTTI_GetID();                                                                             \
    }

    template<class TDstPtr, class TSrc, std::enable_if_t<std::is_base_of_v<TSrc, std::remove_pointer_t<TDstPtr>>, bool> = true>
    inline TDstPtr fe_dynamic_cast(TSrc* src)
    {
        if (src->template FeRTTI_Is<std::remove_pointer_t<TDstPtr>>())
            return reinterpret_cast<TDstPtr>(src);
        return nullptr;
    }
} // namespace FE
