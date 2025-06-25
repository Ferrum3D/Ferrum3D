#include <FeCore/Memory/FiberTempAllocator.h>
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
        festd::string_view storageView{ defineStorage };
        while (!storageView.empty())
        {
            ++defineCount;

            const auto spaceIter = storageView.find_first_of(' ');
            if (spaceIter == storageView.end())
                break;

            storageView = storageView.substr(spaceIter + 1);
        }

        festd::pmr::vector<ShaderDefine> defines{ allocator };
        defines.reserve(defineCount);

        while (!storageView.empty())
        {
            const auto spaceIter = storageView.find_first_of(' ');
            if (spaceIter == storageView.end())
                break;

            const festd::string_view define = storageView.substr(storageView.begin(), spaceIter);
            const auto equalIter = define.find_first_of('=');
            FE_Assert(equalIter != define.end());

            const festd::string_view name = define.substr(define.begin(), equalIter);
            const festd::string_view value = define.substr(equalIter + 1);
            defines.push_back({ name, value });

            storageView = storageView.substr(spaceIter + 1);
        }

        return defines;
    }
} // namespace FE::Graphics::Core
