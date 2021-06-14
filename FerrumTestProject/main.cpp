#include <iostream>
#include <array>
#include <FerrumCore/FeLog.h>
#include <FerrumCore/FeVector.h>
#include <FerrumCore/FeJobSystem.h>
#include <FerrumRenderer/FeRenderer.h>

using namespace Ferrum;

struct TestJob
{
	int source{};
	int index{};
	bool print{};

	int* results{};

	int f(int x) {
		if (x == 0 || x == 1)
			return 1;
		return f(x - 1) * x;
	}

	void Execute() {
		auto result = f(source);
		for (int i = 0; i < 10000; i++)
			result += f(source);
		result = f(source);
		if (print)
			FeLogMsg("Thread #{}; f({}) = {}", std::this_thread::get_id(), source, result);
		results[index] = result;
	}
};

int main() {
	FeLogInit();

	auto results = new int[100000];
	std::vector<std::shared_ptr<FeJobHandle>> jobs;
	auto js = std::make_unique<FeJobSystem>(std::thread::hardware_concurrency());
	for (int i = 0; i < 100000; ++i) {
		TestJob job{};
		job.source = i % 10;
		job.index = i;
		job.results = results;
		job.print = i == 0;
		jobs.push_back(js->Schedule(job, FeJobType::LightJob));
	}

	FeLogMsg("All jobs scheduled");

	for (auto& j : jobs)
		j->Wait();

	FeLogMsg("All completed");
	js->JoinAll();
	FeLogMsg("All joined");

	for (int i = 0; i < 20; ++i) {
		FeLogMsg("results[{}] = {}", i, results[i]);
	}

	/*
	auto window = FeCreateWindow(800, 600);
	window->Init();

	auto device = FeCreateGraphicsDevice(window.get(), FeGraphicsDeviceDesc{});

	while (!window->ShouldClose()) {
		window->PollEvents();
	}

	window->Close();
	*/
}
