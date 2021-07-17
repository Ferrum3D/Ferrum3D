#include "FeAllocatorTests.h"
#include "FeAssetManagerTests.h"
#include "FeECSTests.h"
#include "FeTestFramework.h"
#include "FeVectorTests.h"

using namespace FE;

int main()
{
    InitLogger();

    while (true)
    {
        LogMsg("Enter test category:");
        std::string cat{};
        std::getline(std::cin, cat);

        if (cat == "all")
            FeRunAllTests();
        else if (cat == "")
            break;
        else
            FeRunTestsByCategory(cat);
    }
}
