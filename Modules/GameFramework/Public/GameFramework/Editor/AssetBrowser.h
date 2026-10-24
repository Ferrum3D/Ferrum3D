#pragma once
#include <Core/IO/Assets.h>
#include <Core/IO/Path.h>

namespace FE::Editor
{
    struct AssetEntry final
    {
        IO::AssetID m_id = IO::AssetID::kNull;
        Rtti::TypeID m_type = Rtti::TypeID::kNull;
        festd::string m_name;
        IO::Path m_path;
    };


    struct AssetBrowser final
    {
        void Refresh(const IO::Path& directory);
        IO::AssetID Draw(Rtti::TypeID activationType, bool filterType = false);
        [[nodiscard]] static IO::AssetID ReadAssetFile(const IO::Path& path, Rtti::TypeID type);
        [[nodiscard]] festd::span<const AssetEntry> GetAssets() const
        {
            return m_assets;
        }

    private:
        festd::vector<AssetEntry> m_assets;
        char m_filter[128]{};
    };
} // namespace FE::Editor
