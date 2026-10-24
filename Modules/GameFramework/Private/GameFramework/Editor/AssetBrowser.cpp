#include <Core/IO/AssetFile.h>
#include <Core/IO/FileStream.h>
#include <Core/Serialization/Document.h>
#include <GameFramework/Editor/AssetBrowser.h>
#include <GameFramework/Editor/Controls.h>
#include <imgui.h>

namespace FE::Editor
{
    namespace
    {
        void AddEntry(festd::vector<AssetEntry>& entries, const IO::AssetID id, const Rtti::TypeID type,
                      const festd::string_view name, const IO::Path& path)
        {
            if (!id.IsValid())
                return;

            for (const auto& entry : entries)
            {
                if (entry.m_id == id)
                    return;
            }

            auto& entry = entries.emplace_back();
            entry.m_id = id;
            entry.m_type = type;
            entry.m_path = path;
            entry.m_name = name.empty() ? Fmt::Format("{}", id) : festd::string(name);
        }
    } // namespace


    void AssetBrowser::Refresh(const IO::Path& directory)
    {
        m_assets.clear();
        IO::Directory::TraverseRecursively(directory, "*.asset", [this](const IO::DirectoryEntry& entry) {
            IO::AssetFile file;
            const IO::Path path(entry.m_path);
            if (!IO::LoadAssetDefinition(path, file))
                return true;

            for (const auto& product : file.m_artifacts)
            {
                if (!product.m_isRemoved)
                {
                    AddEntry(m_assets,
                             product.m_assetId,
                             product.m_assetTypeId,
                             Fmt::Format("{} / {}", IO::PathView(path).stem(), product.m_name),
                             path);
                }
            }

            return true;
        });
        IO::Directory::TraverseRecursively(directory, "*.meta", [this](const IO::DirectoryEntry& entry) {
            const IO::Path path(entry.m_path);
            auto file = IO::FileStream::Open(path, IO::OpenMode::kReadOnly);
            if (!file)
                return true;

            IO::ArtifactRecord record;
            if (Serialization::LoadDocument(**file, record) == Serialization::ResultCode::kSuccess)
            {
                const auto* type = Rtti::TypeRegistry::FindType(record.m_assetTypeId);
                const auto name = type ? GetDisplayName(*type) : festd::ascii_view{};
                AddEntry(m_assets, record.m_assetId, record.m_assetTypeId, name, path);
            }

            return true;
        });
    }


    IO::AssetID AssetBrowser::Draw(const Rtti::TypeID activationType, const bool filterType)
    {
        IO::AssetID opened = IO::AssetID::kNull;
        ImGui::InputTextWithHint("##Filter", "Filter assets", m_filter, sizeof(m_filter));
        const float scale = ImGui::GetWindowDpiScale();
        const int columns = Math::Max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / (120.0f * scale)));
        if (!ImGui::BeginTable("AssetGrid", columns))
            return opened;

        for (const AssetEntry& asset : m_assets)
        {
            const bool accepted = !activationType.IsValid() || asset.m_type == activationType;
            if (filterType && !accepted)
                continue;

            if (m_filter[0] && !strstr(asset.m_name.c_str(), m_filter))
                continue;

            ImGui::TableNextColumn();
            ImGui::PushID(Fmt::FixedFormat("{}", asset.m_id).c_str());
            const auto* type = Rtti::TypeRegistry::FindType(asset.m_type);
            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 2);
            ImGui::Button(festd::inline_string(GetIcon(type)).c_str(), ImVec2(100 * scale, 62 * scale));
            ImGui::PopFont();
            if (accepted && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                opened = asset.m_id;

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s\n%s", asset.m_name.c_str(), asset.m_path.data());

            ImGui::TextWrapped("%s", asset.m_name.c_str());
            ImGui::PopID();
        }

        ImGui::EndTable();
        return opened;
    }


    IO::AssetID AssetBrowser::ReadAssetFile(const IO::Path& path, const Rtti::TypeID type)
    {
        IO::AssetFile file;
        if (!IO::LoadAssetDefinition(path, file))
            return IO::AssetID::kNull;

        for (const auto& product : file.m_artifacts)
        {
            if (!product.m_isRemoved && product.m_assetTypeId == type)
                return product.m_assetId;
        }

        return IO::AssetID::kNull;
    }
} // namespace FE::Editor
