#pragma once

namespace FE
{
    class IFeRenderPass
    {
        virtual void Init()    = 0;
        virtual void Execute() = 0;
    };
} // namespace FE
