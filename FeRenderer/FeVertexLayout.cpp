#include "FeVertexLayout.h"

namespace FE
{
    FeVertexLayout::FeVertexLayout(size_t size)
    {
        if (size > 0)
        {
            m_Elements.reserve(size);
        }
    }

    void FeVertexLayout::AddElement(uint32_t componentCount, FeValueType valueType, bool normalized)
    {
        m_Elements.push_back(DL::LayoutElement{ uint32_t(m_Elements.size()), 0, componentCount, (DL::VALUE_TYPE)valueType, normalized });
    }
} // namespace FE
