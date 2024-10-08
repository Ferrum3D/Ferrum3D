﻿#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Strings/String.h>
#include <FeCore/Strings/Unicode.h>
#include <array>
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <string_view>

namespace FE::Fmt
{
    using FE::String;
    using FE::StringSlice;

    struct IValueFormatter
    {
        virtual void Format(String& buffer, void* value) const = 0;
    };

    template<class T>
    struct BasicValueFormatter : IValueFormatter
    {
        virtual void Format(String& buffer, const T& value) const = 0;

        virtual void Format(String& buffer, void* value) const override final
        {
            Format(buffer, *reinterpret_cast<T*>(value));
        }
    };

    template<class T>
    struct ValueFormatter : BasicValueFormatter<T>
    {
        void Format(String& buffer, const T& value) const override
        {
            std::stringstream ss;
            if constexpr (std::is_pointer_v<T> && !std::is_same_v<T, const char*>)
            {
                ss << "0x" << std::hex << reinterpret_cast<size_t>(value);
            }
            else
            {
                ss << value;
            }

            auto v = ss.str();
            buffer.Append(v.data(), static_cast<uint32_t>(v.size()));
        }
    };

    template<>
    struct ValueFormatter<String> : BasicValueFormatter<String>
    {
        void Format(String& buffer, const String& value) const override
        {
            buffer.Append(value);
        }
    };

    template<>
    struct ValueFormatter<StringSlice> : BasicValueFormatter<StringSlice>
    {
        void Format(String& buffer, const StringSlice& value) const override
        {
            buffer.Append(value);
        }
    };

    template<>
    struct ValueFormatter<Env::Name> : BasicValueFormatter<Env::Name>
    {
        void Format(String& buffer, const Env::Name& name) const override
        {
            const Env::Name::Record* pRecord = name.GetRecord();
            buffer.Append(pRecord->Data, pRecord->Size);
        }
    };

    template<>
    struct ValueFormatter<std::string> : BasicValueFormatter<std::string>
    {
        void Format(String& buffer, const std::string& value) const override
        {
            buffer.Append(value.data(), static_cast<uint32_t>(value.size()));
        }
    };

    template<>
    struct ValueFormatter<std::string_view> : BasicValueFormatter<std::string_view>
    {
        void Format(String& buffer, const std::string_view& value) const override
        {
            buffer.Append(value.data(), static_cast<uint32_t>(value.size()));
        }
    };

    template<>
    struct ValueFormatter<UUID> : BasicValueFormatter<UUID>
    {
        void Format(String& buffer, const UUID& value) const override
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
        template<class TFormatter>
        inline TFormatter* GetFormatter()
        {
            static TFormatter instance;
            return &instance;
        }

        struct FormatArg
        {
            inline FormatArg() = default;

            inline FormatArg(void* value, IValueFormatter* formatter) noexcept
            {
                Value = value;
                Formatter = formatter;
            }

            template<class T>
            inline static FormatArg Create(T&& arg) noexcept
            {
                auto* f = GetFormatter<ValueFormatter<std::decay_t<T>>>();
                return FormatArg((void*)&arg, f);
            }

            inline void FormatTo(String& str) const
            {
                Formatter->Format(str, Value);
            }

            void* Value = nullptr;
            IValueFormatter* Formatter = nullptr;
        };

        template<size_t ArgsCount>
        struct FormatArgs
        {
            std::array<FormatArg, ArgsCount> Data;

            inline bool HasUnused() const
            {
                for (const auto& arg : Data)
                    if (arg.Value)
                        return true;
                return false;
            }
        };

        template<size_t ArgsCount>
        void FormatImpl(String& str, StringSlice fmt, FormatArgs<ArgsCount>& args)
        {
            size_t argIndex = 0;
            auto begin = fmt.begin();
            for (auto it = fmt.begin(); it != fmt.end(); ++it)
            {
                if (*it == '{')
                {
                    auto braceIt = it;
                    if (*++it == '{')
                    {
                        str.Append(StringSlice(begin, it));
                        begin = it;
                        begin++;
                        continue;
                    }
                    FE_CORE_ASSERT(*it == '}', ""); // do not accept arguments and indices for now
                    str.Append(StringSlice(begin, braceIt));
                    begin = it;
                    begin++;

                    auto& arg = args.Data[argIndex++];
                    arg.FormatTo(str);
                    arg.Value = nullptr;
                }
                else if (*it == '}')
                {
                    ++it;
                    FE_CORE_ASSERT(*it == '}', "must be escaped");

                    str.Append(StringSlice(begin, it));
                    begin = it;
                    begin++;
                    continue;
                }
            }
            str.Append(StringSlice(begin, fmt.end()));
        }
    } // namespace Internal

    template<class... Args>
    inline String Format(StringSlice fmt, Args&&... args)
    {
        String result;
        Internal::FormatArgs<sizeof...(Args)> formatArgs{ Internal::FormatArg::Create(args)... };
        Internal::FormatImpl<sizeof...(Args)>(result, fmt, formatArgs);
        return result;
    }
} // namespace FE::Fmt
