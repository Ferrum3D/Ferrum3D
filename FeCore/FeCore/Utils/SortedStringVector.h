#pragma once
#include <algorithm>
#include <string>
#include <vector>

namespace FE
{
    template<class TString, class TStrView>
    struct StringToView
    {
    };

    template<>
    struct StringToView<std::string, std::string_view>
    {
        inline std::string_view operator()(const std::string& str)
        {
            return str;
        }
    };

    template<
        class T, class TString = std::string, class TStrView = std::string_view,
        class TAllocator = std::allocator<std::tuple<TString, T>>>
    class SortedStringVector
    {
        std::vector<std::tuple<TString, T>, TAllocator> m_Data;

        inline static constexpr auto Compare = [](const std::tuple<TString, T>& lhs, const TStrView& rhs) -> bool {
            auto& str = std::get<0>(lhs);
            return StringToView<TString, TStrView>{}(str) < rhs;
        };

    public:
        using iterator       = decltype(m_Data.begin());
        using const_iterator = decltype(m_Data.cbegin());

        inline SortedStringVector(TAllocator allocator)
            : m_Data(allocator)
        {
        }

        inline const T& operator[](const TStrView& str) const
        {
            return AtString(str);
        }

        inline T& operator[](const TStrView& str)
        {
            return AtString(str);
        }

        inline auto FindIter(const TString& key) noexcept
        {
            return std::lower_bound(m_Data.begin(), m_Data.end(), StringToView<TString, TStrView>{}(key), Compare);
        }

        inline auto FindCIter(const TString& key) const noexcept
        {
            return std::lower_bound(m_Data.cbegin(), m_Data.cend(), StringToView<TString, TStrView>{}(key), Compare);
        }

        inline auto FindIter(const TStrView& key) noexcept
        {
            return std::lower_bound(m_Data.begin(), m_Data.end(), key, Compare);
        }

        inline auto FindCIter(const TStrView& key) const noexcept
        {
            return std::lower_bound(m_Data.cbegin(), m_Data.cend(), key, Compare);
        }

        inline const T& AtString(const TStrView& str) const
        {
            auto it = FindCIter(str);
            return std::get<1>(*it);
        }

        inline T& AtString(const TStrView& str)
        {
            auto it = FindIter(str);
            return std::get<1>(*it);
        }

        inline std::tuple<TString, T>& Push(const TString& key, const T& value)
        {
            auto it    = FindIter(key);
            size_t idx = it - m_Data.begin();
            if (it != m_Data.end() && std::get<0>(*it) == key)
                std::get<1>(*it) = value;
            else
                m_Data.insert(it, std::make_tuple(key, value));
            return m_Data[idx];
        }

        inline std::tuple<TString, T>& Push(TString&& key, T&& value)
        {
            auto it    = FindIter(key);
            size_t idx = it - m_Data.begin();
            if (it != m_Data.end() && std::get<0>(*it) == key)
                std::get<1>(*it) = value;
            else
                m_Data.insert(it, std::make_tuple(std::move(key), std::move(value)));
            return m_Data[idx];
        }

        inline void Erase(iterator iter)
        {
            m_Data.erase(iter);
        }

        inline iterator begin()
        {
            return m_Data.begin();
        }

        inline iterator end()
        {
            return m_Data.end();
        }

        inline const_iterator begin() const
        {
            return m_Data.cbegin();
        }

        inline const_iterator end() const
        {
            return m_Data.cend();
        }

        inline const_iterator cbegin() const
        {
            return m_Data.cbegin();
        }

        inline const_iterator cend() const
        {
            return m_Data.cend();
        }
    };
} // namespace FE
