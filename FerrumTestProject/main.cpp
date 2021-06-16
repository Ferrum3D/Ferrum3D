#include <iostream>
#include <array>
#include <FerrumCore/FeLog.h>
#include <FerrumCore/FeVector.h>
#include <FerrumCore/FeJobSystem.h>
#include <FerrumRenderer/FeRenderer.h>

using namespace Ferrum;

int main() {
	FeLogInit();

	auto window = FeCreateWindow(800, 600);
	window->Init();

	auto device = FeCreateGraphicsDevice(window.get(), FeGraphicsDeviceDesc{});

	while (!window->ShouldClose()) {
		window->PollEvents();
	}

	window->Close();
}
