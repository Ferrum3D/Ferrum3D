#pragma once
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::Str
{
    template<class TArray>
    void Split(const festd::string_view s, TArray& tokens, const int32_t delimiter = ' ', uint32_t maxTokens = Constants::kMaxU32)
    {
        if (maxTokens == 0)
            return;

        auto current = s.begin();
        while (current != s.end())
        {
            const auto cPos = maxTokens-- == 0 ? s.end() : s.find_first_of(current, delimiter);
            tokens.emplace_back(current.m_iter, cPos.m_iter - current.m_iter);
            current = cPos;
            if (current != s.end())
                ++current;
        }
    }


    inline festd::pmr::vector<festd::string_view> Split(const festd::string_view s, const int32_t delimiter = ' ',
                                                        const uint32_t maxTokens = Constants::kMaxU32,
                                                        std::pmr::memory_resource* allocator = nullptr)
    {
        if (allocator == nullptr)
            allocator = std::pmr::get_default_resource();

        festd::pmr::vector<festd::string_view> tokens{ allocator };
        if (maxTokens < Constants::kMaxU32)
            tokens.reserve(maxTokens);

        Split(s, tokens, delimiter, maxTokens);
        return tokens;
    }


    template<uint32_t TSize>
    festd::fixed_vector<festd::string_view, TSize> SplitFixed(const festd::string_view s, const int32_t delimiter = ' ')
    {
        festd::fixed_vector<festd::string_view, TSize> tokens;
        Split(s, tokens, delimiter, TSize);
        return tokens;
    }


    inline bool Match(const festd::string_view s, const festd::string_view pattern)
    {
        auto sIter = s.begin();
        auto pIter = pattern.begin();

        while (sIter != s.end() && pIter != pattern.end() && *pIter != '*')
        {
            if (*sIter != *pIter && *pIter != '?')
                return false;

            ++sIter;
            ++pIter;
        }

        auto se = s.end();
        auto pe = pattern.end();
        while (sIter != s.end())
        {
            if (pIter != pattern.end())
            {
                if (*pIter == '*')
                {
                    ++pIter;
                    if (pIter == pattern.end())
                        return true;

                    pe = pIter;
                    se = sIter;
                    ++se;
                    continue;
                }

                if (*sIter == *pIter || *pIter == '?')
                {
                    ++sIter;
                    ++pIter;
                    continue;
                }
            }

            pIter = pe;
            sIter = se++;
        }

        while (pIter != pattern.end() && *pIter == '*')
            ++pIter;

        return pIter == pattern.end();
    }


    inline bool MatchAny(const festd::string_view s, const festd::span<const festd::string_view> patterns)
    {
        for (const festd::string_view pattern : patterns)
        {
            if (Match(s, pattern))
                return true;
        }

        return false;
    }
} // namespace FE::Str
