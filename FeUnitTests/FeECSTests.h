#pragma once
#include "FeTestFramework.h"
#include <FeCore/Scene/FeEntity.h>
#include <FeCore/Scene/FeScene.h>

namespace FE
{
	struct FeTestComponent
	{
		int A = 0;

		FeTestComponent(int a) {
			A = a;
		}
	};

	struct FeTestComponent1
	{
		int B = 0;

		FeTestComponent1(int b) {
			B = b;
		}
	};

	FE_TEST(Basic, ECS)
	{
		FeScene scene{};
		auto e1 = scene.CreateEntity();
		auto e2 = scene.CreateEntity();
		auto e3 = scene.CreateEntity();

		e1.AddComponent<FeTestComponent>(1);
		e2.AddComponent<FeTestComponent>(2);
		e3.AddComponent<FeTestComponent>(3);

		std::vector<int> vector{};
		auto iter = scene.GetIterator<FeTestComponent>();
		iter.ForEach([&](FeEntity entity, auto& comp) {
			vector.push_back(comp.A);
		});

		std::sort(vector.begin(), vector.end());
		FT_EXPECT(FeKnownSizeContainerEqual(vector, std::array{ 1, 2, 3 }));
	}

	FE_TEST(MultiComponent, ECS)
	{
		FeScene scene{};
		auto e1 = scene.CreateEntity();
		auto e2 = scene.CreateEntity();
		auto e3 = scene.CreateEntity();

		e1.AddComponent<FeTestComponent>(1);
		e2.AddComponent<FeTestComponent>(2);
		e3.AddComponent<FeTestComponent>(3);

		e1.AddComponent<FeTestComponent1>(1);
		e2.AddComponent<FeTestComponent1>(2);

		std::vector<int> vector{};
		auto iter = scene.GetIterator<FeTestComponent, FeTestComponent1>();
		iter.ForEach([&](FeEntity entity, auto& comp, auto& comp1) {
			vector.push_back(comp.A);
			vector.push_back(comp1.B);
		});

		std::sort(vector.begin(), vector.end());
		FT_EXPECT(FeKnownSizeContainerEqual(vector, std::array{ 1, 1, 2, 2 }));
	}
}
