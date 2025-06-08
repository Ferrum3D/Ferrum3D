#pragma once
#include <FeCore/Modules/Environment.h>
#include <Graphics/Core/AdapterInfo.h>
#include <Graphics/Core/Base/BaseTypes.h>

namespace FE::Graphics::Core
{
    struct DeviceFactory : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(DeviceFactory, "C6CC0410-BB89-484A-8FD7-9DF99AE3CD31");

        virtual ResultCode CreateDevice(Env::Name adapterName) = 0;

        [[nodiscard]] virtual festd::span<const AdapterInfo> EnumerateAdapters() const = 0;
    };
} // namespace FE::Graphics::Core
