#pragma once
#include <cstdint>
#include <cstddef>

namespace FE
{
    using TypeID = std::size_t;

#define FE_RTTI(name)                                                                                                            \
    inline virtual const char* const FERTTI_GetName() const                                                                      \
    {                                                                                                                            \
        return #name;                                                                                                            \
    }                                                                                                                            \
                                                                                                                                 \
    inline virtual TypeID FERTTI_GetID() const                                                                                   \
    {                                                                                                                            \
        return name ::FERTTI_GetSID();                                                                                           \
    }                                                                                                                            \
                                                                                                                                 \
    inline static TypeID FERTTI_GetSID()                                                                                         \
    {                                                                                                                            \
        static int i;                                                                                                            \
        return reinterpret_cast<TypeID>(&i);                                                                                     \
    }                                                                                                                            \
                                                                                                                                 \
    inline static const char* FERTTI_GetSName()                                                                                  \
    {                                                                                                                            \
        return #name;                                                                                                            \
    }                                                                                                                            \
                                                                                                                                 \
    template<class T>                                                                                                            \
    inline bool FERTTI_Is() const                                                                                                \
    {                                                                                                                            \
        return T::FERTTI_GetSID() == FERTTI_GetID();                                                                             \
    }

    template<class TDstPtr, class TSrc>
    inline TDstPtr fe_dynamic_cast(TSrc* src)
    {
        if (src->template FERTTI_Is<std::remove_pointer_t<TDstPtr>>())
            return reinterpret_cast<TDstPtr>(src);
        return nullptr;
    }
} // namespace FE
