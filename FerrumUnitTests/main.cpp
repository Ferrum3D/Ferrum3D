#include "FeTestFramework.h"
#include "FeVectorTests.h"
#include "FeAllocatorTests.h"
#include "FeAssetManagerTests.h"

using namespace Ferrum;

int main() {
	FeLogInit();

	while (true) {
		FeLogMsg("Enter test category:");
		std::string cat{};
		std::getline(std::cin, cat);
		if (cat == "all") {
			FeRunAllTests();
		}
		else if (cat == "") {
			break;
		}
		else {
			FeRunTestsByCategory(cat);
		}
	}
}
