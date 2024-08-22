#pragma once
#include <FeCore/Modules/Environment.h>
#include <HAL/AdapterInfo.h>
#include <HAL/Common/BaseTypes.h>

namespace FE::Graphics::HAL
{
    class DeviceFactory : public Memory::RefCountedObjectBase
    {
    public:
        ~DeviceFactory() override = default;

        FE_RTTI_Class(DeviceFactory, "C6CC0410-BB89-484A-8FD7-9DF99AE3CD31");

        virtual ResultCode CreateDevice(Env::Name adapterName) = 0;

        [[nodiscard]] virtual festd::span<const AdapterInfo> EnumerateAdapters() const = 0;
    };
} // namespace FE::Graphics::HAL
