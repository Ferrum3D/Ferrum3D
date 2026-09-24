#include <AssetBuilder/ArtifactWriter.h>
#include <AssetBuilder/MaterialProcessor.h>

#include <Core/IO/FileStream.h>
#include <Core/Memory/Memory.h>
#include <Graphics/Assets/MaterialAssets.h>
#include <lua.h>
#include <luacode.h>
#include <lualib.h>

namespace FE::AssetBuilder
{
    namespace
    {
        void* AllocateLuau(void*, void* pointer, size_t, const size_t newSize)
        {
            if (newSize == 0)
            {
                Memory::DefaultFree(pointer);
                return nullptr;
            }
            return Memory::DefaultReallocate(pointer, newSize);
        }


        bool ReadSource(const IO::Path& path, festd::vector<std::byte>& bytes)
        {
            auto file = IO::FileStream::Open(path, IO::OpenMode::kReadOnly);
            if (!file || (*file)->Length() > Constants::kMaxU32)
                return false;

            bytes.resize(static_cast<uint32_t>((*file)->Length()));
            return (*file)->ReadToBuffer(bytes.data(), bytes.size()) == bytes.size();
        }


        bool RunChunk(lua_State* state, const char* name, const festd::span<const std::byte> source)
        {
            size_t bytecodeSize = 0;
            char* bytecode = luau_compile(reinterpret_cast<const char*>(source.data()), source.size(), nullptr, &bytecodeSize);
            const int loadResult = luau_load(state, name, bytecode, bytecodeSize, 0);
            ::free(bytecode);
            if (loadResult != 0)
            {
                Logger::LogError("Luau '{}': {}", name, lua_tostring(state, -1));
                lua_pop(state, 1);
                return false;
            }
            if (lua_pcall(state, 0, 1, 0) != 0)
            {
                Logger::LogError("Luau '{}': {}", name, lua_tostring(state, -1));
                lua_pop(state, 1);
                return false;
            }
            return true;
        }


        int RequireEngineModule(lua_State* state)
        {
            const char* name = luaL_checkstring(state, 1);
            constexpr const char* modules[] = { "material", "specializer", "technique", "pipeline", "set" };
            bool knownModule = false;
            for (const char* module : modules)
                knownModule |= strcmp(name, module) == 0;
            if (!knownModule)
            {
                luaL_error(state, "unknown engine material module '%s'", name);
            }

            lua_getglobal(state, "__materialModules");
            lua_getfield(state, -1, name);
            if (!lua_isnil(state, -1))
                return 1;
            lua_pop(state, 1);

            IO::Path path(FE_MATERIAL_LIBRARY_DIR);
            path /= Fmt::FixedFormat("{}.luau", name);
            festd::vector<std::byte> source;
            if (!ReadSource(path, source) || !RunChunk(state, name, source))
            {
                luaL_error(state, "failed to load engine material module '%s'", name);
            }
            lua_pushvalue(state, -1);
            lua_setfield(state, -3, name);
            return 1;
        }


        int Require(lua_State* state)
        {
            const char* name = luaL_checkstring(state, 1);
            constexpr const char* prefix = "@engine/";
            if (strncmp(name, prefix, strlen(prefix)) != 0)
            {
                luaL_error(state, "material scripts may only require @engine modules");
            }
            lua_pushstring(state, name + strlen(prefix));
            lua_replace(state, 1);
            return RequireEngineModule(state);
        }


        struct MaterialVm final
        {
            MaterialVm()
            {
                m_state = lua_newstate(AllocateLuau, nullptr);
                if (m_state == nullptr)
                    return;
                luaL_openlibs(m_state);
                lua_newtable(m_state);
                lua_setglobal(m_state, "__materialModules");
                lua_pushcfunction(m_state, Require, "require");
                lua_setglobal(m_state, "require");
            }

            ~MaterialVm()
            {
                if (m_state)
                    lua_close(m_state);
            }

            lua_State* m_state = nullptr;
        };


        festd::string GetString(lua_State* state, const int table, const char* field)
        {
            lua_getfield(state, table, field);
            festd::string result;
            if (lua_isstring(state, -1))
                result = lua_tostring(state, -1);
            lua_pop(state, 1);
            return result;
        }


        bool GetBoolean(lua_State* state, int table, const char* field);


        Env::Name GetName(lua_State* state, const int table, const char* field)
        {
            lua_getfield(state, table, field);
            Env::Name result;
            if (lua_isstring(state, -1))
                result = Env::Name(lua_tostring(state, -1));
            lua_pop(state, 1);
            return result;
        }


        template<class T, size_t TSize>
        bool ParseEnum(const festd::string_view name, const festd::string_view (&names)[TSize], T& value)
        {
            for (uint32_t index = 0; index < TSize; ++index)
            {
                if (name == names[index])
                {
                    value = static_cast<T>(index);
                    return true;
                }
            }
            return false;
        }


        bool ReadRasterization(lua_State* state, const int table, Graphics::Core::RasterizationState& result)
        {
            constexpr festd::string_view cullModes[] = { "None", "Back", "Front", "BackAndFront" };
            constexpr festd::string_view polygonModes[] = { "Fill", "Line", "Point" };
            Graphics::Core::CullingModeFlags cullMode{};
            Graphics::Core::PolygonMode polygonMode{};
            if (!ParseEnum(GetString(state, table, "cullMode"), cullModes, cullMode)
                || !ParseEnum(GetString(state, table, "polyMode"), polygonModes, polygonMode))
                return false;
            result.m_cullMode = cullMode;
            result.m_polyMode = polygonMode;
            return true;
        }


        bool ReadDepthStencil(lua_State* state, const int table, Graphics::Core::DepthStencilState& result)
        {
            constexpr festd::string_view compareOps[] = { "Never",     "Always",  "Less",     "Equal",
                                                          "LessEqual", "Greater", "NotEqual", "GreaterEqual" };
            Graphics::Core::CompareOp compareOp{};
            if (!ParseEnum(GetString(state, table, "depthCompareOp"), compareOps, compareOp))
                return false;
            result.m_depthCompareOp = compareOp;
            result.m_depthTestEnabled = GetBoolean(state, table, "depthTestEnabled");
            result.m_depthWriteEnabled = GetBoolean(state, table, "depthWriteEnabled");
            result.m_stencilTestEnabled = GetBoolean(state, table, "stencilTestEnabled");
            return true;
        }


        bool ReadBlend(lua_State* state, const int table, Graphics::Core::TargetColorBlending& result)
        {
            constexpr festd::string_view factors[] = { "Zero",
                                                       "One",
                                                       "SrcColor",
                                                       "OneMinusSrcColor",
                                                       "DstColor",
                                                       "OneMinusDstColor",
                                                       "SrcAlpha",
                                                       "OneMinusSrcAlpha",
                                                       "DstAlpha",
                                                       "OneMinusDstAlpha",
                                                       "ConstantColor",
                                                       "OneMinusConstantColor",
                                                       "ConstantAlpha",
                                                       "OneMinusConstantAlpha",
                                                       "SrcAlphaSaturate",
                                                       "Src1Color",
                                                       "OneMinusSrc1Color",
                                                       "Src1Alpha",
                                                       "OneMinusSrc1Alpha" };
            constexpr festd::string_view operations[] = { "Add", "Subtract", "ReverseSubtract", "Min", "Max" };

            Graphics::Core::BlendFactor source{};
            Graphics::Core::BlendFactor destination{};
            Graphics::Core::BlendFactor sourceAlpha{};
            Graphics::Core::BlendFactor destinationAlpha{};
            Graphics::Core::BlendOperation operation{};
            Graphics::Core::BlendOperation alphaOperation{};
            if (!ParseEnum(GetString(state, table, "sourceFactor"), factors, source)
                || !ParseEnum(GetString(state, table, "destinationFactor"), factors, destination)
                || !ParseEnum(GetString(state, table, "sourceAlphaFactor"), factors, sourceAlpha)
                || !ParseEnum(GetString(state, table, "destinationAlphaFactor"), factors, destinationAlpha)
                || !ParseEnum(GetString(state, table, "blendOp"), operations, operation)
                || !ParseEnum(GetString(state, table, "alphaBlendOp"), operations, alphaOperation))
                return false;

            result.m_sourceFactor = source;
            result.m_destinationFactor = destination;
            result.m_sourceAlphaFactor = sourceAlpha;
            result.m_destinationAlphaFactor = destinationAlpha;
            result.m_blendOp = operation;
            result.m_alphaBlendOp = alphaOperation;
            result.m_blendEnabled = GetBoolean(state, table, "blendEnabled");

            lua_getfield(state, table, "colorWrite");
            if (!lua_istable(state, -1))
                return false;
            const uint32_t colorWriteMask = (GetBoolean(state, -1, "R") ? 1u : 0u) | (GetBoolean(state, -1, "G") ? 2u : 0u)
                | (GetBoolean(state, -1, "B") ? 4u : 0u) | (GetBoolean(state, -1, "A") ? 8u : 0u);
            result.m_colorWriteFlags = static_cast<Graphics::Core::ColorComponentFlags>(colorWriteMask);
            lua_pop(state, 1);
            return true;
        }


        bool GetBoolean(lua_State* state, const int table, const char* field)
        {
            lua_getfield(state, table, field);
            const bool result = lua_toboolean(state, -1) != 0;
            lua_pop(state, 1);
            return result;
        }


        bool ReadSpecializers(lua_State* state, const int material, Graphics::MaterialAsset& output)
        {
            lua_getfield(state, material, "specializers");
            if (!lua_istable(state, -1))
                return false;
            const int table = lua_absindex(state, -1);
            lua_pushnil(state);
            while (lua_next(state, table) != 0)
            {
                if (!lua_isstring(state, -2) || !lua_istable(state, -1))
                    return false;
                Graphics::MaterialSpecializerDesc& specializer = output.m_specializers.emplace_back();
                specializer.m_name = Env::Name(lua_tostring(state, -2));
                lua_getfield(state, -1, "default");
                specializer.m_defaultValue = static_cast<int32_t>(lua_tointeger(state, -1));
                lua_pop(state, 1);
                lua_getfield(state, -1, "values");
                if (!lua_istable(state, -1))
                    return false;
                const int valueCount = lua_objlen(state, -1);
                for (int index = 1; index <= valueCount; ++index)
                {
                    lua_rawgeti(state, -1, index);
                    if (!lua_isnumber(state, -1))
                        return false;
                    specializer.m_values.push_back(static_cast<int32_t>(lua_tointeger(state, -1)));
                    lua_pop(state, 1);
                }
                lua_pop(state, 2);
            }
            lua_pop(state, 1);
            festd::sort(output.m_specializers, [](const auto& lhs, const auto& rhs) {
                return lhs.m_name.GetHash() < rhs.m_name.GetHash();
            });
            return true;
        }


        bool ReadParameters(lua_State* state, const int material, Graphics::MaterialAsset& output)
        {
            lua_getfield(state, material, "parameters");
            if (!lua_istable(state, -1))
                return false;
            const int table = lua_absindex(state, -1);
            lua_pushnil(state);
            while (lua_next(state, table) != 0)
            {
                if (!lua_isstring(state, -2) || !lua_istable(state, -1))
                    return false;
                Graphics::MaterialParameterDesc& parameter = output.m_parameters.emplace_back();
                parameter.m_name = Env::Name(lua_tostring(state, -2));
                const festd::string type = GetString(state, -1, "type");
                const festd::string scope = GetString(state, -1, "scope");
                if (type == "Scalar")
                    parameter.m_type = Graphics::MaterialParameterType::kScalar;
                else if (type == "Vector")
                    parameter.m_type = Graphics::MaterialParameterType::kVector;
                else if (type == "Texture2D")
                    parameter.m_type = Graphics::MaterialParameterType::kTexture2D;
                else
                    return false;
                if (scope == "Material")
                    parameter.m_scope = Graphics::MaterialParameterScope::kMaterial;
                else if (scope == "Instance")
                    parameter.m_scope = Graphics::MaterialParameterScope::kInstance;
                else
                    return false;

                lua_getfield(state, -1, "default");
                if (lua_istable(state, -1))
                {
                    float values[4] = {};
                    for (int index = 1; index <= 4; ++index)
                    {
                        lua_rawgeti(state, -1, index);
                        if (lua_isnumber(state, -1))
                            values[index - 1] = static_cast<float>(lua_tonumber(state, -1));
                        lua_pop(state, 1);
                    }
                    parameter.m_defaultValue = Vector4(values[0], values[1], values[2], values[3]);
                }
                lua_pop(state, 2);
            }
            lua_pop(state, 1);
            festd::sort(output.m_parameters, [](const auto& lhs, const auto& rhs) {
                return lhs.m_name.GetHash() < rhs.m_name.GetHash();
            });
            return true;
        }


        bool ReadTechniques(lua_State* state, const int material, Graphics::MaterialAsset& output)
        {
            lua_getfield(state, material, "techniques");
            if (!lua_istable(state, -1))
                return false;
            const int roles = lua_absindex(state, -1);
            lua_pushnil(state);
            while (lua_next(state, roles) != 0)
            {
                if (!lua_isstring(state, -2) || !lua_istable(state, -1))
                    return false;
                const Env::Name role(lua_tostring(state, -2));
                const int variants = lua_absindex(state, -1);
                lua_pushnil(state);
                while (lua_next(state, variants) != 0)
                {
                    if (!lua_isstring(state, -2) || !lua_istable(state, -1))
                        return false;
                    Graphics::MaterialTechniqueDesc& technique = output.m_techniques.emplace_back();
                    technique.m_role = role;
                    technique.m_permutationKey = lua_tostring(state, -2);
                    technique.m_vertexShader = GetName(state, -1, "vertexShader");
                    technique.m_amplificationShader = GetName(state, -1, "amplificationShader");
                    technique.m_meshShader = GetName(state, -1, "meshShader");
                    technique.m_pixelShader = GetName(state, -1, "pixelShader");
                    if (!technique.m_meshShader.IsValid() && !technique.m_vertexShader.IsValid())
                        return false;

                    lua_getfield(state, -1, "rasterization");
                    if (!lua_istable(state, -1))
                        return false;
                    if (!ReadRasterization(state, -1, technique.m_rasterization))
                        return false;
                    lua_pop(state, 1);

                    lua_getfield(state, -1, "depthStencil");
                    if (!lua_istable(state, -1))
                        return false;
                    if (!ReadDepthStencil(state, -1, technique.m_depthStencil))
                        return false;
                    lua_pop(state, 1);

                    lua_getfield(state, -1, "blend");
                    if (!lua_istable(state, -1))
                        return false;
                    Graphics::Core::TargetColorBlending blend;
                    if (!ReadBlend(state, -1, blend))
                        return false;
                    memset(&technique.m_blend, 0, sizeof(technique.m_blend));
                    technique.m_blend.m_targetBlendStates[0] = blend;
                    lua_pop(state, 1);

                    lua_getfield(state, -1, "shaderDefines");
                    if (!lua_istable(state, -1))
                        return false;
                    const int defines = lua_absindex(state, -1);
                    festd::vector<festd::string> defineEntries;
                    lua_pushnil(state);
                    while (lua_next(state, defines) != 0)
                    {
                        if (!lua_isstring(state, -2) || !lua_isstring(state, -1))
                            return false;
                        defineEntries.push_back(Fmt::Format("{}={}", lua_tostring(state, -2), lua_tostring(state, -1)));
                        lua_pop(state, 1);
                    }
                    lua_pop(state, 1);
                    festd::sort(defineEntries, [](const auto& lhs, const auto& rhs) {
                        return lhs < rhs;
                    });
                    for (const festd::string& entry : defineEntries)
                    {
                        if (!technique.m_shaderDefines.empty())
                            technique.m_shaderDefines += " ";
                        technique.m_shaderDefines += entry;
                    }
                    lua_pop(state, 1);
                }
                lua_pop(state, 1);
            }
            lua_pop(state, 1);
            festd::sort(output.m_techniques, [](const auto& lhs, const auto& rhs) {
                if (lhs.m_role != rhs.m_role)
                    return lhs.m_role.GetHash() < rhs.m_role.GetHash();
                return lhs.m_permutationKey < rhs.m_permutationKey;
            });
            return !output.m_techniques.empty();
        }


        bool CompileMaterial(const IO::Path& path, const festd::span<const std::byte> source, Graphics::MaterialAsset& output)
        {
            MaterialVm vm;
            if (vm.m_state == nullptr || !RunChunk(vm.m_state, "Material", source))
                return false;
            if (!lua_istable(vm.m_state, -1))
            {
                Logger::LogError("Material '{}' must return a material table", path);
                return false;
            }
            const int material = lua_absindex(vm.m_state, -1);
            output.m_name = GetString(vm.m_state, material, "name");
            if (output.m_name.empty() || !ReadSpecializers(vm.m_state, material, output)
                || !ReadParameters(vm.m_state, material, output) || !ReadTechniques(vm.m_state, material, output))
            {
                Logger::LogError("Material '{}' contains invalid declarations", path);
                return false;
            }
            return true;
        }
    } // namespace


    bool ValidateMaterialSource(const IO::Path& path, const festd::span<const std::byte> sourceData)
    {
        Graphics::MaterialAsset material;
        return CompileMaterial(path, sourceData, material);
    }


    bool ProcessMaterial(const MaterialProcessSettings& settings)
    {
        Graphics::MaterialAsset material;
        if (!CompileMaterial(settings.m_inputFile, settings.m_sourceData, material))
            return false;

        ArtifactWriter writer(settings.m_outputDirectory, settings.m_assetId, Rtti::GetTypeID<Graphics::MaterialAsset>());
        if (!writer.WriteHeader(material) || !writer.Finish())
            return false;
        *settings.m_resultArtifactId = writer.GetArtifactID();
        return true;
    }


    bool ParseMaterialInstanceSource(const IO::Path& path, const festd::span<const std::byte> sourceData,
                                     Graphics::MaterialInstanceAsset& result)
    {
        MaterialVm vm;
        lua_State* state = vm.m_state;
        if (state == nullptr || !RunChunk(state, "MaterialInstance", sourceData) || !lua_istable(state, -1))
            return false;

        const int instance = lua_absindex(state, -1);
        const festd::string materialId = GetString(state, instance, "material");
        const IO::AssetID materialAssetId(festd::ascii_view(materialId.data(), materialId.size()));
        if (!materialAssetId.IsValid())
        {
            Logger::LogError("Material instance '{}' has an invalid material ID", path);
            return false;
        }
        result.m_material = IO::Link<Graphics::MaterialAsset>(materialAssetId);

        lua_getfield(state, instance, "permutation");
        if (!lua_istable(state, -1))
            return false;
        const int permutation = lua_absindex(state, -1);
        festd::vector<festd::string> choices;
        lua_pushnil(state);
        while (lua_next(state, permutation) != 0)
        {
            if (!lua_isstring(state, -2) || !lua_isnumber(state, -1))
                return false;
            choices.push_back(Fmt::Format("{}={}", lua_tostring(state, -2), static_cast<int32_t>(lua_tointeger(state, -1))));
            lua_pop(state, 1);
        }
        lua_pop(state, 1);
        festd::sort(choices, [](const auto& lhs, const auto& rhs) {
            return lhs < rhs;
        });
        for (const festd::string& choice : choices)
        {
            if (!result.m_permutationKey.empty())
                result.m_permutationKey += ";";
            result.m_permutationKey += choice;
        }

        lua_getfield(state, instance, "parameters");
        if (!lua_istable(state, -1))
            return false;
        const int parameters = lua_absindex(state, -1);
        lua_pushnil(state);
        while (lua_next(state, parameters) != 0)
        {
            if (!lua_isstring(state, -2))
                return false;
            Graphics::MaterialParameterValue& parameter = result.m_parameters.emplace_back();
            parameter.m_name = Env::Name(lua_tostring(state, -2));
            if (lua_isstring(state, -1))
            {
                const IO::AssetID textureId(lua_tostring(state, -1));
                if (!textureId.IsValid())
                    return false;
                parameter.m_texture = IO::Link<Graphics::TextureAsset>(textureId);
            }
            else if (lua_isnumber(state, -1))
            {
                parameter.m_value = Vector4(static_cast<float>(lua_tonumber(state, -1)), 0.0f, 0.0f, 0.0f);
            }
            else if (lua_istable(state, -1))
            {
                float values[4] = {};
                const int count = lua_objlen(state, -1);
                if (count < 1 || count > 4)
                    return false;
                for (int index = 1; index <= count; ++index)
                {
                    lua_rawgeti(state, -1, index);
                    if (!lua_isnumber(state, -1))
                        return false;
                    values[index - 1] = static_cast<float>(lua_tonumber(state, -1));
                    lua_pop(state, 1);
                }
                parameter.m_value = Vector4(values[0], values[1], values[2], values[3]);
            }
            else
            {
                return false;
            }
            lua_pop(state, 1);
        }
        lua_pop(state, 1);
        festd::sort(result.m_parameters, [](const auto& lhs, const auto& rhs) {
            return lhs.m_name.GetHash() < rhs.m_name.GetHash();
        });
        return true;
    }


    bool ProcessMaterialInstance(const MaterialProcessSettings& settings)
    {
        Graphics::MaterialInstanceAsset instance;
        if (!ParseMaterialInstanceSource(settings.m_inputFile, settings.m_sourceData, instance))
            return false;

        ArtifactWriter writer(settings.m_outputDirectory, settings.m_assetId, Rtti::GetTypeID<Graphics::MaterialInstanceAsset>());
        writer.AddDependency(instance.m_material.GetAssetID(), Rtti::GetTypeID<Graphics::MaterialAsset>());
        for (const Graphics::MaterialParameterValue& parameter : instance.m_parameters)
        {
            if (parameter.m_texture.GetAssetID().IsValid())
                writer.AddDependency(parameter.m_texture.GetAssetID(), Rtti::GetTypeID<Graphics::TextureAsset>());
        }
        if (!writer.WriteHeader(instance) || !writer.Finish())
            return false;
        *settings.m_resultArtifactId = writer.GetArtifactID();
        return true;
    }
} // namespace FE::AssetBuilder
