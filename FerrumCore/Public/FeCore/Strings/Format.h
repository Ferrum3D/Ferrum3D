#pragma once
#include <FeCore/Base/Base.h>
#include <concepts>
#include <festd/string.h>
#include <itoa/jeaiii_to_text.h>

FE_PUSH_MSVC_WARNING(4702)
#include <dragonbox/dragonbox_to_chars.h>
FE_POP_MSVC_WARNING()

namespace FE::Fmt
{
    namespace Internal
    {
        template<class>
        inline constexpr bool kAlwaysFalse = false;

        template<class T>
        concept FormattableInteger = std::integral<T> && !std::same_as<T, bool> && !std::same_as<T, char>
            && !std::same_as<T, wchar_t> && !std::same_as<T, char8_t> && !std::same_as<T, char16_t> && !std::same_as<T, char32_t>;

        template<class T>
        concept FormattableFloat = std::same_as<T, float> || std::same_as<T, double>;

        template<class TBuffer, class T>
        concept BufferAppendableValue = !std::is_pointer_v<T> && !std::is_array_v<T> && !std::integral<T>
            && !std::floating_point<T> && requires(TBuffer& buffer, const T& value) { buffer.append(value); };

        template<class TBuffer, class T>
        concept BufferPlusAppendableValue = !std::is_pointer_v<T> && !std::is_array_v<T> && !std::integral<T>
            && !std::floating_point<T> && requires(TBuffer& buffer, const T& value) { buffer += value; };

        inline char* TrimEmptyExp(char* buffer, const char* begin)
        {
            if (buffer - begin < 2)
            {
                return buffer;
            }

            if (*(buffer - 1) == '0' && *(buffer - 2) == 'E')
            {
                return buffer - 2;
            }

            return buffer;
        }
    } // namespace Internal


    template<class TInt>
    struct IntFormatter final
    {
        explicit IntFormatter(const TInt value)
        {
            auto* ptr = jeaiii::to_text_from_integer(m_buffer, value);
            m_size = static_cast<uint8_t>(ptr - m_buffer);
        }

        [[nodiscard]] festd::string_view View() const
        {
            return { m_buffer, m_size };
        }

        char m_buffer[23];
        uint8_t m_size;
    };


    template<class TFloat>
    struct FloatFormatter final
    {
        using Traits = jkj::dragonbox::default_float_traits<TFloat>;
        using Format = Traits::format;

        explicit FloatFormatter(const TFloat value)
        {
            char* ptr = Internal::TrimEmptyExp(jkj::dragonbox::to_chars_n(value, m_buffer), m_buffer);
            m_size = static_cast<uint8_t>(ptr - m_buffer);
        }

        [[nodiscard]] festd::string_view View() const
        {
            return { m_buffer, m_size };
        }

        char m_buffer[jkj::dragonbox::max_output_string_length<Format> + 1];
        uint8_t m_size;
    };


    template<class TBuffer, class T>
    struct ValueFormatter
    {
        void Format(TBuffer&, const T&) const
        {
            // static_assert(Internal::kAlwaysFalse<T>, "No Fmt::ValueFormatter specialization exists for this argument type");
            FE_DebugBreak();
        }
    };


    template<class TBuffer, Internal::FormattableInteger TInt>
    struct ValueFormatter<TBuffer, TInt>
    {
        void Format(TBuffer& buffer, const TInt value) const
        {
            const IntFormatter fmt{ value };
            buffer.append(fmt.View());
        }
    };


    template<class TBuffer, Internal::FormattableFloat TFloat>
    struct ValueFormatter<TBuffer, TFloat>
    {
        void Format(TBuffer& buffer, const TFloat value) const
        {
            const FloatFormatter fmt{ value };
            buffer.append(fmt.View());
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, const char*>
    {
        void Format(TBuffer& buffer, const char* value) const
        {
            FE_Assert(value != nullptr, "Can't format a null C string");
            if (value == nullptr)
                return;

            buffer.append(value, ASCII::Length(value));
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, char*>
    {
        void Format(TBuffer& buffer, const char* value) const
        {
            ValueFormatter<TBuffer, const char*>{}.Format(buffer, value);
        }
    };

    template<class TBuffer, class T>
    struct ValueFormatter<TBuffer, T*>
    {
        void Format(TBuffer& buffer, T* value) const
        {
            ValueFormatter<TBuffer, uintptr_t>{}.Format(buffer, reinterpret_cast<uintptr_t>(value));
        }
    };

    template<class TBuffer, class T>
        requires Internal::BufferAppendableValue<TBuffer, T>
    struct ValueFormatter<TBuffer, T>
    {
        void Format(TBuffer& buffer, const T& value) const
        {
            buffer.append(value);
        }
    };

    template<class TBuffer, class T>
        requires Internal::BufferPlusAppendableValue<TBuffer, T>
        && !Internal::BufferAppendableValue<TBuffer, T>
    struct ValueFormatter<TBuffer, T>
    {
        void Format(TBuffer& buffer, const T& value) const
        {
            buffer += value;
        }
    };

    template<class TBuffer, uint32_t TSize>
    struct ValueFormatter<TBuffer, char[TSize]>
    {
        void Format(TBuffer& buffer, const char (&value)[TSize]) const
        {
            buffer.append(value);
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, Env::Name>
    {
        void Format(TBuffer& buffer, const Env::Name& name) const
        {
            const Env::Name::Record* pRecord = name.GetRecord();
            buffer.append(pRecord->m_data, pRecord->m_size);
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, std::string_view>
    {
        void Format(TBuffer& buffer, const std::string_view& value) const
        {
            buffer.append(value.data(), static_cast<uint32_t>(value.size()));
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, Uuid>
    {
        void Format(TBuffer& buffer, const Uuid& value) const
        {
            static constexpr char kDigits[] = "0123456789ABCDEF";
            int32_t idx = 0;
            buffer.reserve(buffer.size() + 36);

            const auto append = [&](const uint32_t n) {
                for (uint32_t i = 0; i < n; ++i)
                {
                    const uint8_t c = value.m_bytes[idx++];
                    buffer.append(kDigits[(c & 0xF0) >> 4]);
                    buffer.append(kDigits[(c & 0x0F) >> 0]);
                }
            };

            append(4);
            buffer.append('-');
            append(2);
            buffer.append('-');
            append(2);
            buffer.append('-');
            append(2);
            buffer.append('-');
            append(6);
        }
    };

    namespace Internal
    {
        enum class ArgIndexingMode
        {
            kUndetermined,
            kAutomatic,
            kManual,
        };

        template<class TBuffer>
        struct FormatArg
        {
            using FuncType = void (*)(const void*, TBuffer&);

            FuncType pFunc = nullptr;
            const void* pValue = nullptr;

            template<class T>
            static FormatArg Create(T* arg) noexcept
            {
                const auto func = [](const void* value, TBuffer& buffer) {
                    using ValueType = std::remove_cv_t<T>;
                    ValueFormatter<TBuffer, ValueType>{}.Format(buffer, *static_cast<const T*>(value));
                };
                return FormatArg{ func, arg };
            }

            void FormatTo(TBuffer& buffer) const
            {
                pFunc(pValue, buffer);
            }
        };


        template<class TBuffer, size_t TArgCount>
        struct FormatArgs
        {
            festd::array<FormatArg<TBuffer>, TArgCount> m_data;
        };

        template<class TBuffer, size_t TArgCount>
        void FormatImpl(TBuffer& buffer, const festd::string_view fmt, FormatArgs<TBuffer, TArgCount>& args)
        {
            uint32_t nextArgIndex = 0;
            auto indexingMode = ArgIndexingMode::kUndetermined;
            auto begin = fmt.begin();
            const auto end = fmt.end();
            for (auto it = fmt.begin(); it != end; ++it)
            {
                if (*it == '{')
                {
                    auto braceIt = it;
                    ++it;
                    if (it == end)
                    {
                        FE_Assert(false, "Invalid format string: unmatched '{'");
                        buffer.append(festd::string_view(begin, end));
                        return;
                    }

                    if (*it == '{')
                    {
                        buffer.append(festd::string_view(begin, it));
                        begin = it;
                        begin++;
                        continue;
                    }

                    uint32_t argIndex = 0;
                    if (*it != '}')
                    {
                        FE_Assert(indexingMode != ArgIndexingMode::kAutomatic, "Can't switch from automatic to manual indexing");
                        if (indexingMode == ArgIndexingMode::kAutomatic)
                            return;

                        indexingMode = ArgIndexingMode::kManual;
                        bool hasDigits = false;
                        while (it != end)
                        {
                            if (*it <= '9' && *it >= '0')
                            {
                                hasDigits = true;
                                argIndex *= 10;
                                argIndex += static_cast<size_t>(*it) - '0';
                            }
                            else
                            {
                                break;
                            }

                            ++it;
                        }

                        FE_Assert(hasDigits && it != end && *it == '}', "Invalid arg index");
                        if (!hasDigits || it == end || *it != '}')
                        {
                            buffer.append(festd::string_view(begin, end));
                            return;
                        }
                    }
                    else
                    {
                        FE_Assert(indexingMode != ArgIndexingMode::kManual, "Can't switch from manual to automatic indexing");
                        if (indexingMode == ArgIndexingMode::kManual)
                            return;

                        indexingMode = ArgIndexingMode::kAutomatic;
                        argIndex = nextArgIndex++;
                    }

                    FE_Assert(argIndex < TArgCount, "Format argument index out of range");
                    if (argIndex >= TArgCount)
                        return;

                    buffer.append(festd::string_view(begin, braceIt));
                    begin = it;
                    begin++;

                    auto& arg = args.m_data[argIndex];
                    arg.FormatTo(buffer);
                }
                else if (*it == '}')
                {
                    ++it;
                    FE_Assert(it != end && *it == '}', "Invalid format string: '}' must be escaped");
                    if (it == end || *it != '}')
                    {
                        buffer.append(festd::string_view(begin, end));
                        return;
                    }

                    buffer.append(festd::string_view(begin, it));
                    begin = it;
                    begin++;
                }
            }

            buffer.append(festd::string_view(begin, fmt.end()));
        }
    } // namespace Internal


    template<class TBuffer, class... TArgs>
    void FormatTo(TBuffer& buffer, const festd::string_view fmt, TArgs&&... args)
    {
        Internal::FormatArgs<TBuffer, sizeof...(TArgs)> formatArgs{ Internal::FormatArg<TBuffer>::Create(&args)... };
        Internal::FormatImpl<TBuffer, sizeof...(TArgs)>(buffer, fmt, formatArgs);
    }


    template<class... TArgs>
    festd::string Format(festd::string_view fmt, TArgs&&... args)
    {
        festd::string buffer;
        FormatTo<festd::string, TArgs...>(buffer, fmt, std::forward<TArgs>(args)...);
        return buffer;
    }


    template<uint32_t TSize, class... TArgs>
    festd::basic_fixed_string<TSize> FixedFormatSized(festd::string_view fmt, TArgs&&... args)
    {
        festd::basic_fixed_string<TSize> buffer;
        FormatTo<festd::basic_fixed_string<TSize>, TArgs...>(buffer, fmt, std::forward<TArgs>(args)...);
        return buffer;
    }


    template<class... TArgs>
    festd::fixed_string FixedFormat(festd::string_view fmt, TArgs&&... args)
    {
        festd::fixed_string buffer;
        FormatTo<festd::fixed_string, TArgs...>(buffer, fmt, std::forward<TArgs>(args)...);
        return buffer;
    }


    template<class... TArgs>
    Env::Name FormatName(festd::string_view fmt, TArgs&&... args)
    {
        return Env::Name{ FixedFormat(fmt, std::forward<TArgs>(args)...) };
    }
} // namespace FE::Fmt
