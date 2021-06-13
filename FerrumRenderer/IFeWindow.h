#pragma once
#include <cstdint>

namespace Ferrum
{
	class IFeWindow
	{
	public:
		virtual void Init() = 0;
		virtual void PollEvents() = 0;
		virtual bool ShouldClose() = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual void Close() = 0;
	};
}
