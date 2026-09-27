#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/ShaderSpecialization.h>

namespace FE::Graphics::Core
{
    Env::Name CombineDefines(const festd::span<const ShaderDefine> defines)
    {
        FE_PROFILER_ZONE();

        Memory::FiberTempAllocator temp;
        festd::pmr::vector<uint32_t> sortedIndices{ &temp };
        sortedIndices.resize(defines.size());
        festd::iota(sortedIndices, 0);
        festd::sort(sortedIndices, [&defines](const uint32_t lhs, const uint32_t rhs) {
            return defines[lhs].m_name < defines[rhs].m_name;
        });

        uint32_t bytesRequired = 0;
        for (const auto& [name, value] : defines)
        {
            bytesRequired += name.size();
            bytesRequired += value.size();
            bytesRequired += 2; // one for space and one for '='
        }

        festd::pmr::string storage{ &temp };
        storage.reserve(bytesRequired);

        for (const uint32_t defineIndex : sortedIndices)
        {
            if (!storage.empty())
                storage += " ";

            storage += defines[defineIndex].m_name;
            storage += "=";
            storage += defines[defineIndex].m_value;
        }

        return Env::Name{ storage };
    }


    festd::pmr::vector<ShaderDefine> SplitDefines(const Env::Name defineStorage, std::pmr::memory_resource* allocator)
    {
        if (allocator == nullptr)
            allocator = std::pmr::get_default_resource();

        uint32_t defineCount = 0;
        const festd::string_view storageView{ defineStorage };
        festd::string_view countView = storageView;
        while (!countView.empty())
        {
            ++defineCount;

            const auto spaceIter = countView.find_first_of(' ');
            if (spaceIter == countView.end())
                break;

            countView = countView.substr(spaceIter + 1);
        }

        festd::pmr::vector<ShaderDefine> defines{ allocator };
        defines.reserve(defineCount);

        festd::string_view remainingView = storageView;
        while (!remainingView.empty())
        {
            const auto spaceIter = remainingView.find_first_of(' ');
            const festd::string_view define =
                spaceIter == remainingView.end() ? remainingView : remainingView.substr(remainingView.begin(), spaceIter);
            const auto equalIter = define.find_first_of('=');
            FE_Assert(equalIter != define.end());

            const festd::string_view name = define.substr(define.begin(), equalIter);
            const festd::string_view value = define.substr(equalIter + 1);
            defines.push_back({ name, value });

            if (spaceIter == remainingView.end())
                break;

            remainingView = remainingView.substr(spaceIter + 1);
        }

        return defines;
    }
} // namespace FE::Graphics::Core
