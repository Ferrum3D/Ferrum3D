#include <FeCore/ECS/EntityArchetype.h>

namespace FE::ECS
{
    EntityArchetype::EntityArchetype(const ArraySlice<ComponentType>& layout)
    {
        m_Layout.Assign(layout.Data(), layout.Data() + layout.Length());
        InitInternal();
    }

    void EntityArchetype::InitInternal()
    {
        m_Layout.SortByMember(&ComponentType::Alignment, true);

        for (auto& componentType : m_Layout)
        {
            m_EntitySize += componentType.AlignedSize();
            HashCombine(m_HashCode, componentType);
        }
    }
} // namespace FE::ECS
