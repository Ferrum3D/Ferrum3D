#include <FeCore/ECS/EntityArchetype.h>

namespace FE::ECS
{
    EntityArchetype::EntityArchetype(const ArraySlice<ComponentType>& layout)
    {
        m_Layout.Assign(layout.Data(), layout.Data() + layout.Length());
    }
} // namespace FE::ECS
