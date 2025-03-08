#pragma once
#include <FeCore/IO/PathParser.h>
#include <festd/string.h>

namespace FE::IO
{
    inline constexpr uint32_t kMaxPathLength = 260;

    namespace Internal
    {
        using PathBase =
            FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::DefaultFixedStringStorage<kMaxPathLength>>>;
    }


    struct PathView final
    {
        using Iter = FE::Internal::StrIterator;

        PathView() = default;

        explicit PathView(const festd::string_view view)
        {
            Parse(view);
        }

        [[nodiscard]] const char* data() const
        {
            return m_data;
        }

        [[nodiscard]] uint32_t size() const
        {
            return m_size;
        }

        [[nodiscard]] bool has_root_name() const
        {
            return m_hasRootName;
        }

        [[nodiscard]] bool has_root_directory() const
        {
            return m_hasRootDirectory;
        }

        [[nodiscard]] bool has_root_path() const
        {
            return m_hasRootDirectory || m_hasRootName;
        }

        [[nodiscard]] bool has_relative_path() const
        {
            return m_hasRelativePath;
        }

        [[nodiscard]] bool has_filename() const
        {
            return m_filenameSize > 0;
        }

        [[nodiscard]] bool has_stem() const
        {
            return m_filenameSize - m_extensionSize > 0;
        }

        [[nodiscard]] bool has_extension() const
        {
            return m_extensionSize > 0;
        }

        [[nodiscard]] bool is_absolute() const
        {
            return m_hasRootDirectory || m_hasRootName;
        }

        [[nodiscard]] bool is_relative() const
        {
            return !m_hasRootDirectory && !m_hasRootName;
        }

        [[nodiscard]] festd::string_view root_name() const
        {
            return m_hasRootName ? festd::string_view{ m_data, 2 } : festd::string_view{};
        }

        [[nodiscard]] festd::string_view root_directory() const
        {
            return m_hasRootDirectory ? "/" : festd::string_view{};
        }

        [[nodiscard]] festd::string_view root_path() const
        {
            if (m_hasRootName)
            {
                if (m_hasRootDirectory)
                    return festd::string_view{ m_data, 3 };

                return festd::string_view{ m_data, 2 };
            }

            return root_directory();
        }

        [[nodiscard]] festd::string_view parent_directory() const
        {
            return festd::string_view{ m_data, m_size - m_filenameSize - m_extensionSize };
        }

        [[nodiscard]] festd::string_view relative_path() const
        {
            if (!m_hasRelativePath)
                return festd::string_view{};

            if (!m_hasRootName)
                return festd::string_view{ m_data, m_size };

            const char* afterRoot = PathParser::SkipPathRoot(m_data, m_size);
            const char* relativeStart = PathParser::SkipSeparators(afterRoot, m_size - static_cast<uint32_t>(afterRoot - m_data));
            return festd::string_view{ relativeStart, m_size - static_cast<uint32_t>(relativeStart - m_data) };
        }

        [[nodiscard]] festd::string_view filename() const
        {
            const uint32_t fullFilenameSize = m_filenameSize + m_extensionSize;
            return festd::string_view{ m_data + m_size - fullFilenameSize, fullFilenameSize };
        }

        [[nodiscard]] festd::string_view stem() const
        {
            const uint32_t fullFilenameSize = m_filenameSize + m_extensionSize;
            return festd::string_view{ m_data + m_size - fullFilenameSize, m_filenameSize };
        }

        [[nodiscard]] festd::string_view extension() const
        {
            return festd::string_view{ m_data + m_size - m_extensionSize, m_extensionSize };
        }

        [[nodiscard]] Iter begin() const
        {
            return Iter{ m_data };
        }

        [[nodiscard]] Iter end() const
        {
            return Iter{ m_data + m_size };
        }

    private:
        void Parse(const festd::string_view view)
        {
            m_data = view.data();
            m_size = view.size();

            FE_Assert(m_size <= kMaxPathLength, "Path too long");

            const char* afterRoot = PathParser::SkipPathRoot(m_data, m_size);
            const char* relativeStart = PathParser::SkipSeparators(afterRoot, m_size - static_cast<uint32_t>(afterRoot - m_data));
            m_hasRootName = afterRoot > m_data;
            m_hasRootDirectory = relativeStart > afterRoot;
            m_hasRelativePath = m_data + m_size > relativeStart;

            const auto reverseLastSeparatorIter = eastl::find_if(view.rbegin(), view.rend(), [](const int32_t codepoint) {
                return PathParser::IsPathSeparator(codepoint);
            });

            const auto lastSeparatorIter = reverseLastSeparatorIter.base();
            const auto dotIter = eastl::find_if(lastSeparatorIter, view.end(), [](const int32_t codepoint) {
                return codepoint == '.';
            });

            if (dotIter == lastSeparatorIter)
            {
                m_extensionSize = 0;
                m_filenameSize = static_cast<uint32_t>(view.end().m_iter - dotIter.m_iter);
            }
            else
            {
                m_extensionSize = static_cast<uint32_t>(view.end().m_iter - dotIter.m_iter);
                m_filenameSize = static_cast<uint32_t>(dotIter.m_iter - lastSeparatorIter.m_iter);
            }
        }

        const char* m_data = nullptr;
        uint32_t m_size = 0;

        union
        {
            struct
            {
                uint32_t m_filenameSize : 10;
                uint32_t m_extensionSize : 6;
                uint32_t m_hasRootName : 1;
                uint32_t m_hasRootDirectory : 1;
                uint32_t m_hasRelativePath : 1;
            };

            uint32_t m_flags = 0;
        };
    };


    //! @brief A fixed string class that represents a file system path.
    struct Path final : private Internal::PathBase
    {
        using Iter = FE::Internal::StrIterator;

        using value_type = char;

        using Base = Internal::PathBase;

        using Base::reserve;
        using Base::resize;
        using Base::Base::assign;

        using Base::push_back;
        using Base::Base::append;

        using Base::data;
        using Base::empty;
        using Base::length;
        using Base::size;

        using Base::begin;
        using Base::end;

        Path() = default;

        Path(const char* str, const uint32_t size = 0)
            : Base(str, size ? size : ASCII::Length(str))
        {
        }

        Path(const festd::string_view view)
            : Base(view.data(), view.size())
        {
        }

        using Base::operator Env::Name;
        using Base::operator festd::string_view;

        void append(festd::string_view path);
    };


    inline void Path::append(const festd::string_view path)
    {
        if (empty())
            return;

        if (PathParser::IsAbsolutePath(path.data(), path.size()))
        {
            assign(path.data(), path.size());
            return;
        }

        const char lastByte = data()[size() - 1];
        if (!PathParser::IsPathSeparator(lastByte))
            push_back('/');

        const char* appendData = PathParser::SkipSeparators(path.data(), path.size());
        const uint32_t skippedBytesCount = static_cast<uint32_t>(appendData - path.data());
        append(appendData, path.size() - skippedBytesCount);
    }


    inline Path& operator/=(Path& lhs, const festd::string_view rhs)
    {
        lhs.append(rhs);
        return lhs;
    }


    inline Path operator/(const Path& lhs, const festd::string_view rhs)
    {
        Path result = lhs;
        result /= rhs;
        return result;
    }


    template<class TFunc>
    void TraversePath(const festd::string_view path, const TFunc& func)
    {
        const char* data = path.data();
        const uint32_t size = path.size();

        const char* iter = PathParser::SkipPathRoot(data, size);
        func(festd::string_view{ data, static_cast<uint32_t>(iter - data) });

        while (iter < data + size)
        {
            iter = PathParser::SkipSeparators(iter, size - static_cast<uint32_t>(iter - data));
            const char* nextIter = PathParser::SkipUntilSeparator(iter, size - static_cast<uint32_t>(iter - data));
            func(festd::string_view{ iter, static_cast<uint32_t>(nextIter - iter) });
            iter = nextIter;
        }
    }


    namespace Directory
    {
        //! @brief Get the current working directory for the current process.
        Path GetCurrentDirectory();

        //! @brief Change the current working directory for the current process.
        void SetCurrentDirectory(festd::string_view path);

        //! @brief Get the directory where the executable image of the current process is located.
        Path GetExecutableDirectory();

        //! @brief Collapse relative directories like '..', and replace all backslashes with forward slashes.
        Path Normalize(festd::string_view path);
    } // namespace Directory
} // namespace FE::IO
