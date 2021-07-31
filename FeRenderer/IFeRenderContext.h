#pragma once
#include "IFeRenderTarget.h"
#include <FeCore/Math/Vector4.h>

namespace FE
{
    class IFeRenderContext
    {
    public:
        virtual void SetRenderTarget(IFeRenderTarget* renderTarget) = 0;
        virtual void ClearRenderTarget(float4 color, float depth)   = 0;
    };
} // namespace FE
