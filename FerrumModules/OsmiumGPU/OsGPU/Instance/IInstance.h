#pragma once
#include <FeCore/Modules/Environment.h>
#include <OsGPU/Common/BaseTypes.h>

namespace FE::Osmium
{
    class IAdapter;

    class IInstance : public Memory::RefCountedObjectBase
    {
    public:
        ~IInstance() override = default;

        FE_RTTI_Class(IInstance, "C6CC0410-BB89-484A-8FD7-9DF99AE3CD31");

        [[nodiscard]] virtual const eastl::vector<Rc<IAdapter>>& GetAdapters() const = 0;
    };
} // namespace FE::Osmium
