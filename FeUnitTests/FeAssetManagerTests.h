#pragma once
#include <fstream>
#include <FeCore/Assets/FeAssetManager.h>
#include "FeTestFramework.h"

namespace FE
{
	FE_TEST(Basic, AssetManager)
	{
		std::string name = "FeAssetManagerTestFile.txt";
		std::string content = "file content";
		std::ofstream file(name);
		for (int i = 0; i < 1000; ++i) {
			file << content << "\n";
		}

		AssetManager mngr{};
		auto a1 = mngr.LoadRawAsset(name);
		auto a2 = mngr.LoadRawAsset(name);
		for (size_t i = 0; i < content.length(); ++i) {
			FT_EXPECT_MSG(a1.Read<char>()[i] == content[i], FeFormatString("index: {}", i));
		}
		FT_EXPECT(a1.Read<char>() == a2.Read<char>());
	}

	FE_TEST(Weakness, AssetManager)
	{
		std::string name = "FeAssetManagerTestFile.txt";
		std::string content = "file content";
		std::ofstream file(name);
		for (int i = 0; i < 1000; ++i) {
			file << content << "\n";
		}

		AssetManager mngr{};
		{
			auto getsDeleted = mngr.LoadRawAsset(name);
		}

		auto a = mngr.LoadRawAsset(name);
		for (size_t i = 0; i < content.length(); ++i) {
			FT_EXPECT_MSG(a.Read<char>()[i] == content[i], FeFormatString("index: {}", i));
		}
	}
}
