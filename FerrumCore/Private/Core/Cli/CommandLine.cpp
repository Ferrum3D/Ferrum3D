#include <Core/Cli/CommandLine.h>

#include <Core/Strings/Encoding.h>
#include <Core/Strings/Format.h>

namespace FE::Cli
{
    namespace
    {
        [[nodiscard]] festd::string_view GetAttribute(const festd::span<const Rtti::Attribute> attributes,
                                                      const festd::ascii_view key)
        {
            for (const Rtti::Attribute& attribute : attributes)
            {
                if (attribute.m_key == key)
                    return festd::string_view{ attribute.m_value };
            }
            return {};
        }


        [[nodiscard]] bool IsDerivedFrom(const Rtti::Type& type, const Rtti::TypeID baseType)
        {
            if (type.m_id == baseType)
                return true;

            for (const Rtti::TypeID typeID : type.m_baseTypes)
            {
                if (typeID == baseType)
                    return true;
            }
            return false;
        }


        void AppendInferredName(festd::pmr::string& result, festd::string_view name, const bool stripMemberPrefix)
        {
            if (stripMemberPrefix && name.starts_with("m_"))
                name = name.substr(2);

            for (uint32_t index = 0; index < name.size(); ++index)
            {
                const char value = name.data()[index];
                if (value == '_')
                {
                    result += '-';
                    continue;
                }

                if (ASCII::IsUpper(value) && index > 0)
                    result += '-';
                result += ASCII::ToLower(value);
            }
        }


        void AppendCommandName(festd::pmr::string& result, const Rtti::Type& type)
        {
            const festd::string_view explicitName = GetAttribute(type.m_attributes, "Cli::Name");
            if (!explicitName.empty())
                result.append(explicitName);
            else
                AppendInferredName(result, festd::string_view{ type.m_name }, false);
        }


        void AppendOptionName(festd::pmr::string& result, const Rtti::FieldInfo& field)
        {
            const festd::string_view explicitName = GetAttribute(field.m_attributes, "Cli::Name");
            if (!explicitName.empty())
                result.append(explicitName);
            else
                AppendInferredName(result, festd::string_view{ field.m_name }, true);
        }


        [[nodiscard]] bool NameMatches(festd::string_view name, const Rtti::Type& type)
        {
            festd::fixed_string inferred;
            const festd::string_view explicitName = GetAttribute(type.m_attributes, "Cli::Name");
            if (!explicitName.empty())
                return explicitName == name;

            for (uint32_t index = 0; index < type.m_name.length(); ++index)
            {
                const char value = type.m_name[index];
                if (ASCII::IsUpper(value) && index > 0)
                    inferred += '-';
                inferred += ASCII::ToLower(value);
            }
            return festd::string_view{ inferred } == name;
        }


        [[nodiscard]] bool NameMatches(festd::string_view name, const Rtti::FieldInfo& field)
        {
            const festd::string_view explicitName = GetAttribute(field.m_attributes, "Cli::Name");
            if (!explicitName.empty())
                return explicitName == name;

            festd::fixed_string inferred;
            festd::string_view fieldName{ field.m_name };
            if (fieldName.starts_with("m_"))
                fieldName = fieldName.substr(2);

            for (uint32_t index = 0; index < fieldName.size(); ++index)
            {
                const char value = fieldName.data()[index];
                if (value == '_')
                    inferred += '-';
                else
                {
                    if (ASCII::IsUpper(value) && index > 0)
                        inferred += '-';
                    inferred += ASCII::ToLower(value);
                }
            }
            return festd::string_view{ inferred } == name;
        }


        [[nodiscard]] bool HasParent(const Rtti::Type& type, const Rtti::Type& parent)
        {
            const festd::string_view parentName = GetAttribute(type.m_attributes, "Cli::Parent");
            return parentName == festd::string_view{ parent.m_name }
            || parentName == festd::string_view{ parent.m_qualifiedName };
        }


        [[nodiscard]] const Rtti::Type* FindParentType(const Rtti::Type& type)
        {
            const festd::string_view parentName = GetAttribute(type.m_attributes, "Cli::Parent");
            if (parentName.empty())
                return nullptr;

            if (const Rtti::Type* typeByName =
                    Rtti::TypeRegistry::FindType(festd::ascii_view{ parentName.data(), parentName.size() }))
                return typeByName;

            const Rtti::Type* result = nullptr;
            for (const Rtti::Type& candidate : Rtti::TypeRegistry::GetTypes())
            {
                if (festd::string_view{ candidate.m_name } != parentName)
                    continue;
                FE_Assert(result == nullptr, "Ambiguous Cli::Parent type name");
                result = &candidate;
            }
            return result;
        }


        [[nodiscard]] const Rtti::Type* FindChildType(const Rtti::Type& parent, const festd::string_view name)
        {
            const Rtti::Type* result = nullptr;
            for (const Rtti::Type& type : Rtti::TypeRegistry::GetTypes())
            {
                if (!IsDerivedFrom(type, Subcommand::TypeID) || !HasParent(type, parent) || !NameMatches(name, type))
                    continue;

                FE_Assert(result == nullptr, "Duplicate CLI subcommand name");
                result = &type;
            }
            return result;
        }
    } // namespace


    namespace Internal
    {
        struct ParserState final
        {
            ParserState(void* parser, const Rtti::Type& parserType, std::pmr::memory_resource* allocator,
                        const festd::span<const festd::string_view> commandLine)
                : m_parser(static_cast<Parser*>(parser))
                , m_allocator(allocator)
                , m_commandLine(commandLine)
            {
                FE_Assert(parser != nullptr);
                FE_Assert(allocator != nullptr);
                FE_Assert(IsDerivedFrom(parserType, Parser::TypeID));

                m_currentCommand = static_cast<Command*>(parser);
                m_currentCommand->m_cliType = &parserType;
            }

            void SetError(const festd::string_view error)
            {
                if (!m_parser->m_error.empty())
                    return;

                char* storage = Memory::AllocateArray<char>(m_allocator, error.size());
                memcpy(storage, error.data(), error.size());
                m_parser->m_error = festd::string_view{ storage, error.size() };
            }

            void SetUnexpectedArgumentError(const festd::string_view argument)
            {
                const festd::fixed_string error = Fmt::FixedFormat("Unexpected argument '{}'", argument);
                SetError(festd::string_view{ error });
            }

            [[nodiscard]] const Rtti::FieldInfo* FindOption(const festd::string_view name) const
            {
                const Rtti::FieldInfo* result = nullptr;
                for (const Rtti::FieldInfo& field : m_currentCommand->GetCliType().m_fields)
                {
                    if ((field.m_type != Rtti::GetTypeID<Flag>() && field.m_type != Rtti::GetTypeID<Option>())
                        || !NameMatches(name, field))
                    {
                        continue;
                    }

                    FE_Assert(result == nullptr, "Duplicate CLI option name");
                    result = &field;
                }
                return result;
            }

            bool ParseOption(const uint32_t argumentIndex)
            {
                const festd::string_view argument = m_commandLine[argumentIndex];
                const char* separator = nullptr;
                for (uint32_t index = 2; index < argument.size(); ++index)
                {
                    if (argument.data()[index] == '=')
                    {
                        separator = argument.data() + index;
                        break;
                    }
                }

                const uint32_t nameSize =
                    separator ? static_cast<uint32_t>(separator - argument.data()) - 2 : argument.size() - 2;
                const festd::string_view name{ argument.data() + 2, nameSize };
                const Rtti::FieldInfo* field = FindOption(name);
                if (field == nullptr)
                {
                    const festd::fixed_string error = Fmt::FixedFormat("Unknown option '--{}'", name);
                    SetError(festd::string_view{ error });
                    return false;
                }

                if (field->m_type == Rtti::GetTypeID<Flag>())
                {
                    if (separator != nullptr)
                    {
                        const festd::fixed_string error = Fmt::FixedFormat("Flag '--{}' does not accept a value", name);
                        SetError(festd::string_view{ error });
                        return false;
                    }

                    Flag value = field->Get<Flag>(m_currentCommand);
                    if (value.m_value)
                    {
                        const festd::fixed_string error = Fmt::FixedFormat("Option '--{}' was specified more than once", name);
                        SetError(festd::string_view{ error });
                        return false;
                    }
                    value.m_value = true;
                    field->Set(m_currentCommand, value);
                    return true;
                }

                Option value = field->Get<Option>(m_currentCommand);
                if (value.m_hasValue)
                {
                    const festd::fixed_string error = Fmt::FixedFormat("Option '--{}' was specified more than once", name);
                    SetError(festd::string_view{ error });
                    return false;
                }

                if (separator != nullptr)
                {
                    value.m_value = festd::string_view{ separator + 1, argument.data() + argument.size() };
                }
                else
                {
                    if (argumentIndex + 1 >= m_commandLine.size() || m_commandLine[argumentIndex + 1].starts_with("--"))
                    {
                        const festd::fixed_string error = Fmt::FixedFormat("Option '--{}' requires a value", name);
                        SetError(festd::string_view{ error });
                        return false;
                    }
                    value.m_value = m_commandLine[++m_argumentIndex];
                }

                value.m_hasValue = true;
                field->Set(m_currentCommand, value);
                return true;
            }

            bool ParseSubcommand(const festd::string_view argument)
            {
                const Rtti::Type* type = FindChildType(m_currentCommand->GetCliType(), argument);
                if (type == nullptr)
                {
                    SetUnexpectedArgumentError(argument);
                    return false;
                }

                FE_Assert(type->m_defaultConstructor != nullptr, "CLI subcommands must be default constructible");
                void* storage = m_allocator->allocate(type->m_size, type->m_alignment);
                type->m_defaultConstructor(storage);

                auto* command = static_cast<Command*>(storage);
                command->m_cliType = type;
                m_currentCommand->m_subcommand = command;
                m_currentCommand = command;
                return true;
            }

            void Run()
            {
                for (m_argumentIndex = 0; m_argumentIndex < m_commandLine.size(); ++m_argumentIndex)
                {
                    const festd::string_view argument = m_commandLine[m_argumentIndex];
                    if (argument == "--")
                    {
                        if (m_argumentIndex + 1 < m_commandLine.size())
                            SetUnexpectedArgumentError(m_commandLine[m_argumentIndex + 1]);
                        return;
                    }

                    if (argument.starts_with("--"))
                    {
                        if (argument.size() == 2 || !ParseOption(m_argumentIndex))
                            return;
                    }
                    else if (!ParseSubcommand(argument))
                    {
                        return;
                    }
                }
            }

            Parser* m_parser;
            std::pmr::memory_resource* m_allocator;
            festd::span<const festd::string_view> m_commandLine;
            Command* m_currentCommand;
            uint32_t m_argumentIndex = 0;
        };


        void Parse(void* parser, const Rtti::Type& parserType, std::pmr::memory_resource* allocator,
                   const festd::span<const festd::string_view> commandLine)
        {
            ParserState state{ parser, parserType, allocator, commandLine };
            state.Run();
        }
    } // namespace Internal


    festd::pmr::string BuildHelp(std::pmr::memory_resource* allocator, const festd::string_view executableName,
                                 const Command& command)
    {
        festd::pmr::string result{ allocator };
        result.append("Usage: ");
        result.append(executableName);

        const Rtti::Type* commandPath[16];
        uint32_t commandPathSize = 0;
        const Rtti::Type* currentType = &command.GetCliType();
        while (currentType && IsDerivedFrom(*currentType, Subcommand::TypeID))
        {
            FE_Assert(commandPathSize < festd::size(commandPath));
            commandPath[commandPathSize++] = currentType;
            currentType = FindParentType(*currentType);
        }

        while (commandPathSize > 0)
        {
            result += ' ';
            AppendCommandName(result, *commandPath[--commandPathSize]);
        }

        bool hasSubcommands = false;
        for (const Rtti::Type& type : Rtti::TypeRegistry::GetTypes())
        {
            if (IsDerivedFrom(type, Subcommand::TypeID) && HasParent(type, command.GetCliType()))
            {
                hasSubcommands = true;
                break;
            }
        }

        if (hasSubcommands)
            result.append(" [command]");
        if (!command.GetCliType().m_fields.empty())
            result.append(" [options]");
        result.append("\n");

        if (hasSubcommands)
        {
            result.append("\nCommands:\n");
            for (const Rtti::Type& type : Rtti::TypeRegistry::GetTypes())
            {
                if (!IsDerivedFrom(type, Subcommand::TypeID) || !HasParent(type, command.GetCliType()))
                    continue;

                result.append("  ");
                const uint32_t nameBegin = result.size();
                AppendCommandName(result, type);
                const uint32_t nameSize = result.size() - nameBegin;
                result.append(nameSize < 14 ? 14 - nameSize : 1, ' ');
                result.append(GetAttribute(type.m_attributes, "Cli::Description"));
                result += '\n';
            }
        }

        if (!command.GetCliType().m_fields.empty())
        {
            result.append("\nOptions:\n");
            for (const Rtti::FieldInfo& field : command.GetCliType().m_fields)
            {
                if (field.m_type != Rtti::GetTypeID<Flag>() && field.m_type != Rtti::GetTypeID<Option>())
                    continue;

                result.append("  --");
                const uint32_t nameBegin = result.size();
                AppendOptionName(result, field);
                if (field.m_type == Rtti::GetTypeID<Option>())
                {
                    result.append(" <");
                    const festd::string_view valueName = GetAttribute(field.m_attributes, "Cli::ValueName");
                    result.append(valueName.empty() ? festd::string_view{ "value" } : valueName);
                    result += '>';
                }
                const uint32_t nameSize = result.size() - nameBegin;
                result.append(nameSize < 18 ? 18 - nameSize : 1, ' ');
                result.append(GetAttribute(field.m_attributes, "Cli::Description"));
                result += '\n';
            }
        }

        return result;
    }
} // namespace FE::Cli
