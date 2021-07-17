#pragma once
#include "FeGraphicsDevice.h"
#include "IFeVertexLayout.h"
#include <vector>

namespace FE
{
    class FeVertexLayout : public IFeVertexLayout
    {
        std::vector<Diligent::LayoutElement> m_Elements{};

    public:
        FeVertexLayout(size_t size);

        virtual void AddElement(uint32_t componentCount, FeValueType valueType, bool normalized) override;

        inline Diligent::LayoutElement* GetElements()
        {
            return m_Elements.data();
        }

        inline uint32_t GetElementCount()
        {
            return uint32_t(m_Elements.size());
        }
    };
} // namespace FE
