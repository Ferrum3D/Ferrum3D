#pragma once
#include <FeCore/Containers/List.h>
#include <OsGPU/Common/BaseTypes.h>

namespace FE::Osmium
{
    struct InstanceDesc
    {
        FE_STRUCT_RTTI(InstanceDesc, "20125FD5-EFCB-426E-B1EE-50DF51457171");

        const char* ApplicationName;

        inline InstanceDesc() = default;

        inline explicit InstanceDesc(const char* applicationName)
            : ApplicationName(applicationName)
        {
        }
    };

    class IAdapter;

    class IInstance : public IObject
    {
    public:
        ~IInstance() override = default;

        FE_CLASS_RTTI(IInstance, "C6CC0410-BB89-484A-8FD7-9DF99AE3CD31");

        [[nodiscard]] virtual const List<Rc<IAdapter>>& GetAdapters() const = 0;
    };

    typedef void (*AttachEnvironmentProc)(Env::Internal::IEnvironment* environment);
    typedef IInstance* (*CreateGraphicsAPIInstanceProc)(InstanceDesc desc, GraphicsAPI api);
} // namespace FE::Osmium
