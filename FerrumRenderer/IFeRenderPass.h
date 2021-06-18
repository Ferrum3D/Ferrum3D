#pragma once

namespace Ferrum
{
	class IFeRenderPass
	{
		virtual void Init() = 0;
		virtual void Execute() = 0;
	};
}
