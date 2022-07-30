#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/IO/FileStream.h>
#include <FeCore/Memory/Memory.h>

#define RAPIDJSON_SSE2
#define RAPIDJSON_PARSE_DEFAULT_FLAGS kParseCommentsFlag
#include <rapidjson/document.h>

namespace FE::Assets
{
    inline constexpr StringSlice HeaderTemplate = "feassetc - Ferrum3D v{}.{}.{} Asset compiler\n";

    inline constexpr StringSlice CopyrightMessage = "Nikita Dubovikov, Ferrum3D (C) 2022\n";

    inline constexpr StringSlice HelpMessage = R"(usage: feassetc [--version | -v] [--help | -h] <command> [<args>]
Supported commands:
    compile [-D<key>=<value>...] <filepaths>...
            This command compiles asset files to the format supported by the engine. There must exist a file with name
            `<filepath>.meta.json` containing all the metadata needed for the asset loading. You can also specify
            additional metadata with -D options.
            The compiled assets will be written to `<filepath>.compiled.pak` files.

    build   [--release] [--archive-size=<megabytes>] <index-filepath>
            This command incrementally builds the entire project asset database. All asset paths must be contained in the
            specified `FerrumAssetIndex.json` file and have corresponding `*.meta.json` files. The command emits a FerrumAssets
            file and `*.compiled.pak` files for each asset (in dev mode) or a set of large `asset-archive-*.pak` archive files
            (in release mode).
)";

    void PrintHeader()
    {
        Console::PrintToStdout(Fmt::Format(HeaderTemplate, FerrumVersion.Major, FerrumVersion.Minor, FerrumVersion.Patch));
    }

    template<AssetMetadataType T>
    bool TryConvertArrayToVector(const rapidjson::Value& value, typename CppTypeForAssetMetadataType<T>::Type& vector)
    {
        auto arr            = value.GetArray();
        const USize vecSize = T == AssetMetadataType::Vector3 ? 3 : 4;
        if (arr.Size() != vecSize)
        {
            return false;
        }

        for (USize i = 0; i < arr.Size(); ++i)
        {
            if (!arr[i].IsFloat())
            {
                return false;
            }

            vector(i) = arr[i].GetFloat();
        }

        return true;
    }

    List<AssetMetadataField> ReadMetadataForAsset(const StringSlice& assetFilePath, IAssetLoader** assetLoader)
    {
        auto metadataPath = Fmt::Format("{}.meta.json", assetFilePath);
        if (!IO::File::Exists(metadataPath))
        {
            return {};
        }

        auto contents = IO::File::ReadAllText(metadataPath);

        rapidjson::Document doc;
        if (doc.ParseInsitu(contents.Data()).HasParseError())
        {
            USize lineNumber = 1;
            for (auto c : StringSlice(contents.Data(), contents.Data() + doc.GetErrorOffset()))
            {
                if (c == '\n')
                {
                    lineNumber++;
                }
            }

            FE_FATAL_ERROR("Error while parsing asset metadata in file: {} at line: {}", metadataPath, lineNumber);
        }

        FE_EXPECT_OR_FATAL_ERROR(doc.HasMember("AssetType") && doc["AssetType"].IsString(),
                                 "Asset type not found or has invalid format in asset metadata in file: {}",
                                 metadataPath);

        StringSlice assetTypeStr = doc["AssetType"].GetString();
        Assets::AssetType assetType;
        FE_EXPECT_OR_FATAL_ERROR(assetTypeStr.TryConvertTo(assetType),
                                 "Asset type has invalid format in asset metadata in file: {}. It must be a UUID",
                                 metadataPath);

        auto* loader = SharedInterface<Assets::IAssetManager>::Get()->GetAssetLoader(assetType);
        if (assetLoader)
        {
            *assetLoader = loader;
        }

        auto metadata = loader->GetAssetMetadataFields().ToList();
        for (auto& field : metadata)
        {
            auto fieldSet = false;
            if (doc.HasMember(field.GetKey().Data()))
            {
                auto& value = doc[field.GetKey().Data()];
                switch (value.GetType())
                {
                case rapidjson::kNullType:
                    FE_LOG_WARNING("Asset metadata in file: {}: `Null` is not a valid metadata field.", metadataPath);
                    break;
                case rapidjson::kFalseType:
                    if (field.GetType() == AssetMetadataType::Bool)
                    {
                        field.SetValue<AssetMetadataType::Bool>(false);
                        fieldSet = true;
                    }
                    break;
                case rapidjson::kTrueType:
                    if (field.GetType() == AssetMetadataType::Bool)
                    {
                        field.SetValue<AssetMetadataType::Bool>(true);
                        fieldSet = true;
                    }
                    break;
                case rapidjson::kObjectType:
                    FE_LOG_WARNING("Asset metadata in file: {}: `Object` is not a valid metadata field.", metadataPath);
                    break;
                case rapidjson::kArrayType:
                    {
                        if (field.GetType() == AssetMetadataType::Vector4)
                        {
                            Vector4F vec4;
                            if (TryConvertArrayToVector<AssetMetadataType::Vector4>(value, vec4))
                            {
                                field.SetValue<AssetMetadataType::Vector4>(vec4);
                                fieldSet = true;
                            }
                        }
                        else
                        {
                            Vector3F vec3;
                            if (TryConvertArrayToVector<AssetMetadataType::Vector3>(value, vec3))
                            {
                                field.SetValue<AssetMetadataType::Vector3>(vec3);
                                fieldSet = true;
                            }
                        }
                        if (!fieldSet)
                        {
                            FE_LOG_WARNING("Asset metadata in file: {}: `Array` is not a valid metadata field.", metadataPath);
                        }
                    }
                    break;
                case rapidjson::kStringType:
                    {
                        StringSlice s = value.GetString();
                        UUID uuid;
                        if (s.TryConvertTo(uuid) && field.GetType() == AssetMetadataType::UUID)
                        {
                            field.SetValue<AssetMetadataType::UUID>(uuid);
                            fieldSet = true;
                        }
                        else if (field.GetType() == AssetMetadataType::String)
                        {
                            field.SetValue<AssetMetadataType::String>(s);
                            fieldSet = true;
                        }
                    }
                    break;
                case rapidjson::kNumberType:
                    if (field.GetType() == AssetMetadataType::UInt)
                    {
                        field.SetValue<AssetMetadataType::UInt>(value.GetUint64());
                        fieldSet = true;
                    }
                    else if (field.GetType() == AssetMetadataType::Int)
                    {
                        field.SetValue<AssetMetadataType::Int>(value.GetInt64());
                        fieldSet = true;
                    }
                    else if (field.GetType() == AssetMetadataType::Int)
                    {
                        field.SetValue<AssetMetadataType::Float>(value.GetFloat());
                        fieldSet = true;
                    }
                    break;
                default:
                    break;
                }
            }

            if (fieldSet)
            {
                doc.RemoveMember(field.GetKey().Data());
            }
            else
            {
                FE_EXPECT_OR_FATAL_ERROR(
                    field.IsOptional(), "A required parameter not found in asset metadata in file: {}", metadataPath);
                continue;
            }
        }

        List<StringSlice> unusedMembers;
        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
        {
            unusedMembers.Push(it->name.GetString());
        }

        FE_LOG_WARNING("These fields in asset metadata file {} where unused: ", metadataPath, String::Join(", ", unusedMembers));

        return metadata;
    }

    void ProcessOneFile(const StringSlice& file, IO::IStream* outStream,
                        const ArraySlice<std::pair<String, String>>& additionalMetadata)
    {
        IAssetLoader* loader = nullptr;

        FE_ASSERT(additionalMetadata.Empty()); // TODO

        auto metadata = ReadMetadataForAsset(file, &loader);

        auto fileHandle = MakeShared<IO::FileHandle>();
        auto stream     = MakeShared<IO::FileStream>(fileHandle);
        FE_IO_ASSERT(stream->Open(file, IO::OpenMode::ReadOnly));

        Shared<AssetStorage> storage = loader->CreateStorage();
        storage->ReleaseStrongRef();
        loader->LoadRawAsset(metadata, storage.GetRaw(), stream.GetRaw());

        loader->SaveAsset(storage.GetRaw(), outStream);
    }

    int CompileCommand(const ArraySlice<StringSlice>& args)
    {
        List<StringSlice> files;
        List<std::pair<String, String>> additionalMetadata;
        for (auto& arg : args)
        {
            if (arg.StartsWith("-D"))
            {
                auto [key, value] = arg(2, arg.Size()).Split('=').AsTuple<2>();
                additionalMetadata.Push(std::make_pair(key, value));
            }
            else
            {
                files.Push(arg);
            }
        }

        for (auto& file : files)
        {
            auto fileHandle = MakeShared<IO::FileHandle>();
            auto stream     = MakeShared<IO::FileStream>(fileHandle);
            FE_IO_ASSERT(stream->Open(Fmt::Format("{}.compiled.pak", file), IO::OpenMode::Create));

            ProcessOneFile(file, stream.GetRaw(), additionalMetadata);
        }

        return 0;
    }

    int BuildCommand(const ArraySlice<StringSlice>& args)
    {
        FE_LOG_ERROR("Building not implemented!");
        Console::PrintToStdout(String::Join("\n", args));
        return 1;
    }

    int RunCompiler(int argc, char** argv)
    {
        auto logger        = MakeShared<Debug::ConsoleLogger>();
        auto assetManager  = MakeShared<AssetManager>();
        auto assetProvider = MakeShared<AssetProviderDev>();
        auto assetRegistry = MakeShared<AssetRegistry>();
        assetProvider->AttachRegistry(assetRegistry);
        assetManager->AttachAssetProvider(assetProvider);

        List<StringSlice> args;
        args.Reserve(argc - 1);
        for (USize i = 1; i < argc; ++i)
        {
            args.Push(argv[i]);
        }

        if (args.Size() == 0 || args.Contains("--help") || args.Contains("-h"))
        {
            PrintHeader();
            Console::PrintToStdout(HelpMessage);
            return 0;
        }

        if (args.Contains("--version") || args.Contains("-v"))
        {
            PrintHeader();
            Console::PrintToStdout(CopyrightMessage);
            return 0;
        }

        if (args[0] == "compile")
        {
            return CompileCommand(ArraySlice(args)(1, args.Size()));
        }
        else if (args[0] == "build")
        {
            return BuildCommand(ArraySlice(args)(1, args.Size()));
        }

        FE_LOG_ERROR("Invalid command: {}", args[0]);
        return 1;
    }
} // namespace FE::Assets

int main(int argc, char** argv)
{
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    int code = FE::Assets::RunCompiler(argc, argv);
    FE::Console::PrintToStdout("\n");
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
    return code;
}
