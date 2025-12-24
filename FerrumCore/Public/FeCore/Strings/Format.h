#pragma once
#include <EASTL/tuple.h>
#include <FeCore/Base/Base.h>
#include <festd/string.h>
#include <itoa/jeaiii_to_text.h>

FE_PUSH_MSVC_WARNING(4702)
#include <dragonbox/dragonbox_to_chars.h>
FE_POP_MSVC_WARNING()

namespace FE::Fmt
{
    namespace Internal
    {
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

        template<class TBuffer, class TInt>
        struct IntegralValueFormatter
        {
            void Format(TBuffer& buffer, const TInt value) const
            {
                char buf[24];
                auto* ptr = jeaiii::to_text_from_integer(buf, value);
                buffer.append(buf, static_cast<uint32_t>(ptr - buf));
            }
        };
    } // namespace Internal


    template<class TBuffer, class T>
    struct ValueFormatter
    {
    };


    // clang-format off
    template<class TBuffer> struct ValueFormatter<TBuffer, int8_t> : public Internal::IntegralValueFormatter<TBuffer, int8_t> {};
    template<class TBuffer> struct ValueFormatter<TBuffer, uint8_t> : public Internal::IntegralValueFormatter<TBuffer, uint8_t> {};
    template<class TBuffer> struct ValueFormatter<TBuffer, int16_t> : public Internal::IntegralValueFormatter<TBuffer, int16_t> {};
    template<class TBuffer> struct ValueFormatter<TBuffer, uint16_t> : public Internal::IntegralValueFormatter<TBuffer, uint16_t> {};
    template<class TBuffer> struct ValueFormatter<TBuffer, int32_t> : public Internal::IntegralValueFormatter<TBuffer, int32_t> {};
    template<class TBuffer> struct ValueFormatter<TBuffer, uint32_t> : public Internal::IntegralValueFormatter<TBuffer, uint32_t> {};
    template<class TBuffer> struct ValueFormatter<TBuffer, int64_t> : public Internal::IntegralValueFormatter<TBuffer, int64_t> {};
    template<class TBuffer> struct ValueFormatter<TBuffer, uint64_t> : public Internal::IntegralValueFormatter<TBuffer, uint64_t> {};
    // clang-format on


    template<class TBuffer>
    struct ValueFormatter<TBuffer, float>
    {
        void Format(TBuffer& buffer, const float value) const
        {
            char buf[jkj::dragonbox::max_output_string_length<jkj::dragonbox::ieee754_binary32>];
            char* ptr = Internal::TrimEmptyExp(jkj::dragonbox::to_chars_n(value, buf), buf);
            buffer.append(buf, static_cast<uint32_t>(ptr - buf));
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, double>
    {
        void Format(TBuffer& buffer, const double value) const
        {
            char buf[jkj::dragonbox::max_output_string_length<jkj::dragonbox::ieee754_binary64>];
            char* ptr = Internal::TrimEmptyExp(jkj::dragonbox::to_chars_n(value, buf), buf);
            buffer.append(buf, static_cast<uint32_t>(ptr - buf));
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, const char*>
    {
        void Format(TBuffer& buffer, const char* value) const
        {
            buffer.append(value, ASCII::Length(value));
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, festd::string>
    {
        void Format(TBuffer& buffer, const festd::string& value) const
        {
            buffer += value;
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, festd::string_view>
    {
        void Format(TBuffer& buffer, festd::string_view value) const
        {
            buffer += value;
        }
    };

    template<class TBuffer, uint32_t TSize>
    struct ValueFormatter<TBuffer, festd::basic_fixed_string<TSize>>
    {
        void Format(TBuffer& buffer, const festd::basic_fixed_string<TSize>& value) const
        {
            buffer += value;
        }
    };

    template<class TBuffer, uint32_t TSize>
    struct ValueFormatter<TBuffer, char[TSize]>
    {
        void Format(TBuffer& buffer, const char (&value)[TSize]) const
        {
            buffer += value;
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
                    ValueFormatter<TBuffer, std::remove_const_t<T>>{}.Format(buffer,
                                                                             *static_cast<const std::remove_cv_t<T>*>(value));
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
            std::array<FormatArg<TBuffer>, TArgCount> Data;
        };

        template<class TBuffer, size_t TArgCount>
        void FormatImpl(TBuffer& buffer, const festd::string_view fmt, FormatArgs<TBuffer, TArgCount>& args)
        {
            size_t argIndex = 0;
            bool autoIndex = true;
            auto begin = fmt.begin();
            for (auto it = fmt.begin(); it != fmt.end(); ++it)
            {
                if (*it == '{')
                {
                    auto braceIt = it;
                    if (*++it == '{')
                    {
                        buffer.append(begin, it);
                        begin = it;
                        begin++;
                        continue;
                    }
                    if (*it != '}')
                    {
                        FE_Assert(!autoIndex || argIndex == 0, "Can't switch from automatic to manual indexing");
                        argIndex = 0;
                        autoIndex = false;
                        while (true)
                        {
                            FE_Assert(it != fmt.end(), "Invalid arg index");
                            if (*it <= '9' && *it >= '0')
                            {
                                argIndex *= 10;
                                argIndex += static_cast<size_t>(*it) - '0';
                            }
                            else
                            {
                                break;
                            }

                            ++it;
                        }

                        FE_Assert(*it == '}', "Invalid arg index");
                    }
                    buffer.append(begin, braceIt);
                    begin = it;
                    begin++;

                    auto& arg = args.Data[argIndex];
                    arg.FormatTo(buffer);
                    if (autoIndex)
                        ++argIndex;
                }
                else if (*it == '}')
                {
                    ++it;
                    FE_Assert(*it == '}', "must be escaped");

                    buffer.append(begin, it);
                    begin = it;
                    begin++;
                    continue;
                }
            }
            buffer.append(begin, fmt.end());
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
