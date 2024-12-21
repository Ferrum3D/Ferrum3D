#pragma once
#include <EASTL/tuple.h>
#include <FeCore/Base/Base.h>
#include <FeCore/Strings/FixedString.h>
#include <FeCore/Strings/String.h>
#include <FeCore/Strings/Unicode.h>
#include <array>
#include <cstdlib>
#include <itoa/jeaiii_to_text.h>
#include <ostream>
#include <sstream>
#include <string_view>

FE_PUSH_MSVC_WARNING(4702)
#include <dragonbox/dragonbox_to_chars.h>
FE_POP_MSVC_WARNING

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

        template<class TBuffer>
        inline void FormatIntegral(TBuffer& buffer, uint64_t value)
        {
            char buf[24];
            auto* ptr = jeaiii::to_text_from_integer(buf, value);
            buffer.Append(buf, static_cast<uint32_t>(ptr - buf));
        }

        template<class TBuffer>
        inline void FormatIntegral(TBuffer& buffer, int64_t value)
        {
            char buf[24];
            auto* ptr = jeaiii::to_text_from_integer(buf, value);
            buffer.Append(buf, static_cast<uint32_t>(ptr - buf));
        }
    } // namespace Internal


    template<class TBuffer, class T>
    struct ValueFormatter
    {
        void Format(TBuffer& buffer, const T& value) const
        {
            if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
            {
                Internal::FormatIntegral(buffer, static_cast<int64_t>(value));
            }
            else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
            {
                Internal::FormatIntegral(buffer, static_cast<uint64_t>(value));
            }
            else
            {
                FE_CORE_ASSERT(false, "Not implemented");
            }
        }
    };


    template<class TBuffer>
    struct ValueFormatter<TBuffer, float>
    {
        void Format(TBuffer& buffer, float value) const
        {
            char buf[jkj::dragonbox::max_output_string_length<jkj::dragonbox::ieee754_binary32>];
            char* ptr = Internal::TrimEmptyExp(jkj::dragonbox::to_chars_n(value, buf), buf);
            buffer.Append(buf, static_cast<uint32_t>(ptr - buf));
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, double>
    {
        void Format(TBuffer& buffer, double value) const
        {
            char buf[jkj::dragonbox::max_output_string_length<jkj::dragonbox::ieee754_binary64>];
            char* ptr = Internal::TrimEmptyExp(jkj::dragonbox::to_chars_n(value, buf), buf);
            buffer.Append(buf, static_cast<uint32_t>(ptr - buf));
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, const char*>
    {
        void Format(TBuffer& buffer, const char* value) const
        {
            buffer.Append(value);
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, String>
    {
        void Format(TBuffer& buffer, const String& value) const
        {
            buffer.Append(value);
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, StringSlice>
    {
        void Format(TBuffer& buffer, StringSlice value) const
        {
            buffer.Append(value);
        }
    };

    template<class TBuffer, uint32_t TSize>
    struct ValueFormatter<TBuffer, FixedString<TSize>>
    {
        void Format(TBuffer& buffer, const FixedString<TSize>& value) const
        {
            buffer.Append(value);
        }
    };

    template<class TBuffer, uint32_t TSize>
    struct ValueFormatter<TBuffer, char[TSize]>
    {
        void Format(TBuffer& buffer, const char (&value)[TSize]) const
        {
            buffer.Append(value);
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, Env::Name>
    {
        void Format(TBuffer& buffer, const Env::Name& name) const
        {
            const Env::Name::Record* pRecord = name.GetRecord();
            buffer.Append(pRecord->m_data, pRecord->m_size);
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, std::string_view>
    {
        void Format(TBuffer& buffer, const std::string_view& value) const
        {
            buffer.Append(value.data(), static_cast<uint32_t>(value.size()));
        }
    };

    template<class TBuffer>
    struct ValueFormatter<TBuffer, UUID>
    {
        void Format(TBuffer& buffer, const UUID& value) const
        {
            static char digits[] = "0123456789ABCDEF";
            int32_t idx = 0;
            buffer.Reserve(buffer.Size() + 36);
            auto append = [&](uint32_t n) {
                for (uint32_t i = 0; i < n; ++i)
                {
                    uint8_t c = value.Data[idx++];
                    buffer.Append(digits[(c & 0xF0) >> 4]);
                    buffer.Append(digits[(c & 0x0F) >> 0]);
                }
            };

            append(4);
            buffer.Append('-');
            append(2);
            buffer.Append('-');
            append(2);
            buffer.Append('-');
            append(2);
            buffer.Append('-');
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
            inline static FormatArg Create(T* arg) noexcept
            {
                const auto func = [](const void* value, TBuffer& buffer) {
                    ValueFormatter<TBuffer, std::remove_const_t<T>>{}.Format(buffer,
                                                                             *static_cast<const std::remove_cv_t<T>*>(value));
                };
                return FormatArg{ func, arg };
            }

            inline void FormatTo(TBuffer& buffer) const
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
        void FormatImpl(TBuffer& buffer, StringSlice fmt, FormatArgs<TBuffer, TArgCount>& args)
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
                        buffer.Append(StringSlice(begin, it));
                        begin = it;
                        begin++;
                        continue;
                    }
                    if (*it != '}')
                    {
                        FE_CORE_ASSERT(!autoIndex || argIndex == 0, "Can't switch from automatic to manual indexing");
                        argIndex = 0;
                        autoIndex = false;
                        while (true)
                        {
                            FE_CORE_ASSERT(it != fmt.end(), "Invalid arg index");
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

                        FE_CORE_ASSERT(*it == '}', "Invalid arg index");
                    }
                    buffer.Append(StringSlice(begin, braceIt));
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
                    FE_CORE_ASSERT(*it == '}', "must be escaped");

                    buffer.Append(StringSlice(begin, it));
                    begin = it;
                    begin++;
                    continue;
                }
            }
            buffer.Append(StringSlice(begin, fmt.end()));
        }
    } // namespace Internal

    template<class TBuffer, class... TArgs>
    inline void FormatTo(TBuffer& buffer, StringSlice fmt, TArgs&&... args)
    {
        Internal::FormatArgs<TBuffer, sizeof...(TArgs)> formatArgs{ Internal::FormatArg<TBuffer>::Create(&args)... };
        Internal::FormatImpl<TBuffer, sizeof...(TArgs)>(buffer, fmt, formatArgs);
    }

    template<class... TArgs>
    inline String Format(StringSlice fmt, TArgs&&... args)
    {
        String buffer;
        FormatTo<String, TArgs...>(buffer, fmt, std::forward<TArgs>(args)...);
        return buffer;
    }

    template<uint32_t TSize, class... TArgs>
    inline FixedString<TSize> FixedFormatSized(StringSlice fmt, TArgs&&... args)
    {
        FixedString<TSize> buffer;
        FormatTo<FixedString<TSize>, TArgs...>(buffer, fmt, std::forward<TArgs>(args)...);
        return buffer;
    }

    template<class... TArgs>
    inline FixStr256 FixedFormat(StringSlice fmt, TArgs&&... args)
    {
        FixStr256 buffer;
        FormatTo<FixStr256, TArgs...>(buffer, fmt, std::forward<TArgs>(args)...);
        return buffer;
    }
} // namespace FE::Fmt
