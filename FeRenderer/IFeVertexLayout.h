#pragma once
#include <FeCore/Math/FeVectorMath.h>
#include <FeCore/Utils/CoreUtils.h>

namespace FE
{
    // clang-format off
	FE_ENUM(FeValueType)
	{
		Undefined = 0,
		Int8,
		Int16,
		Int32,
		Uint8,
		Uint16,
		Uint32,
		Float16,
		Float32
	};
    // clang-format on

    class IFeVertexLayout
    {
    public:
        virtual void AddElement(uint32_t componentCount, FeValueType valueType, bool normalized) = 0;

        template<class T>
        inline void AddElement()
        {
            static_assert(false, "Type is unsupported for vertex layouts");
        }

        template<>
        inline void AddElement<float3>()
        {
            AddElement(3, FeValueType::Float32, false);
        }

        template<>
        inline void AddElement<float4>()
        {
            AddElement(4, FeValueType::Float32, false);
        }
    };
} // namespace FE
