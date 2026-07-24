#pragma once
#include <Core/Env/Environment.h>
#include <Graphics/Core/AdapterInfo.h>
#include <Graphics/Core/BaseTypes.h>

namespace FE::Graphics::Core
{
    struct Device;

    struct DeviceFactory : public Memory::RefCountedObjectBase
    {
        FE_RTTI("C6CC0410-BB89-484A-8FD7-9DF99AE3CD31");

        virtual Rc<Device> CreateDevice(Env::Name adapterName) = 0;

        [[nodiscard]] virtual festd::span<const AdapterInfo> EnumerateAdapters() const = 0;

        [[nodiscard]] GraphicsApi GetApi() const
        {
            return m_api;
        }

        static void Init(GraphicsApi api);
        static void Shutdown();

        static DeviceFactory& Get();

    protected:
        GraphicsApi m_api = GraphicsApi::kNone;
    };
} // namespace FE::Graphics::Core
