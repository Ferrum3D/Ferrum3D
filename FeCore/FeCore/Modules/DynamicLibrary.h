#pragma once
#include <FeCore/Memory/Object.h>
#include <FeCore/Strings/StringSlice.h>
#include <FeCore/Strings/String.h>

namespace FE
{
    class DynamicLibrary : public Object<IObject>
    {
        void* m_NativeHandle = nullptr;
        String m_FullName;

        void* GetFunctionImpl(StringSlice functionName);

    public:
        FE_CLASS_RTTI(DynamicLibrary, "3A50F5DD-9055-4AD3-ABB5-7737644EB87C");

        ~DynamicLibrary() override;
        DynamicLibrary(StringSlice fileName);

        template<class FPtr>
        inline FPtr GetFunction(StringSlice functionName)
        {
            void* proc = GetFunctionImpl(functionName);
            return proc ? reinterpret_cast<FPtr>(proc) : nullptr;
        }
    };
}
