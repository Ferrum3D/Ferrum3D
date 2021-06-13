#pragma once

namespace Ferrum
{
	enum class FeRenderBackend
	{
		None,
		Direct3D11,
		Direct3D12,
		Vulkan
	};

	struct FeGraphicsDeviceDesc
	{
		FeRenderBackend Backend{ FeRenderBackend::Direct3D12 };
	};

	class IFeGraphicsDevice
	{
	public:

	};
}
