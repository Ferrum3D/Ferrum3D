#pragma once
#include <Core/Memory/Memory.h>
#include <Core/RTTI/Reflection.h>
#include <festd/span.h>
#include <festd/string.h>

namespace FE::Cli
{
    namespace Internal
    {
        struct ParserState;
        void Parse(void* parser, const Rtti::Type& parserType, std::pmr::memory_resource* allocator,
                   festd::span<const festd::string_view> commandLine);
    } // namespace Internal


    festd::span<const festd::string_view> GetArgs();

    bool Check(festd::string_view argument);
    festd::optional<festd::string_view> GetValue(festd::string_view argument);


    struct Flag final
    {
        explicit operator bool() const
        {
            return m_value;
        }

    private:
        friend Internal::ParserState;
        bool m_value = false;
    };


    struct Option final
    {
        explicit operator bool() const
        {
            return m_hasValue;
        }

        [[nodiscard]] festd::string_view Get() const
        {
            FE_Assert(m_hasValue);
            return m_value;
        }

    private:
        friend Internal::ParserState;
        festd::string_view m_value;
        bool m_hasValue = false;
    };


    struct Command
    {
        FE_RTTI_Reflect("01DDD116-D090-49E4-98B9-4100CFB50509");

        template<class T>
        [[nodiscard]] const T* GetSubcommand() const
        {
            return Rtti::Cast<const T*>(m_subcommand);
        }

        [[nodiscard]] const Rtti::Type& GetCliType() const
        {
            FE_Assert(m_cliType != nullptr);
            return *m_cliType;
        }

        virtual ~Command() = default;

        const Command* m_subcommand = nullptr;

    private:
        friend Internal::ParserState;
        const Rtti::Type* m_cliType = nullptr;
    };


    struct Subcommand : public Command
    {
        FE_RTTI_Reflect("0F01E827-C9D9-43F0-8969-47BF6D035B82");
    };


    struct Parser : public Command
    {
        FE_RTTI_Reflect("C3C49654-FE98-4CE6-B759-06D7EF4EAA0F");

        [[nodiscard]] bool IsValid() const
        {
            return m_error.empty();
        }

        [[nodiscard]] festd::string_view GetError() const
        {
            return m_error;
        }

    private:
        friend Internal::ParserState;
        festd::string_view m_error;
    };


    template<class TParser>
        requires std::derived_from<TParser, Parser>
    TParser Parse(std::pmr::memory_resource* allocator, const festd::span<const festd::string_view> commandLine)
    {
        TParser result;
        Internal::Parse(&result, Rtti::GetType<TParser>(), allocator, commandLine);
        return result;
    }


    festd::pmr::string BuildHelp(std::pmr::memory_resource* allocator, festd::string_view executableName, const Command& command);
} // namespace FE::Cli

FE_RTTI_Reflect(FE::Cli::Flag, "9FF56E68-8757-43E2-8826-ACF3419F16FD");
FE_RTTI_Reflect(FE::Cli::Option, "301BC248-FF31-43DB-B285-926623B3326F");
