#pragma once
#include <FeCore/Strings/String.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    class DynamicLibrary : public Memory::RefCountedObjectBase
    {
        void* m_NativeHandle = nullptr;
        String m_FullName;

        void* GetFunctionImpl(StringSlice functionName);

    public:
        FE_CLASS_RTTI(DynamicLibrary, "3A50F5DD-9055-4AD3-ABB5-7737644EB87C");

        inline DynamicLibrary() = default;
        ~DynamicLibrary() override;

        bool LoadFrom(StringSlice fileName);
        void Unload();

        template<class FPtr>
        inline FPtr GetFunction(StringSlice functionName)
        {
            void* proc = GetFunctionImpl(functionName);
            return proc ? reinterpret_cast<FPtr>(proc) : nullptr;
        }
    };
} // namespace FE
