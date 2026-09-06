#pragma once
#include <Core/IO/Assets.h>

namespace FE::IO
{
    struct Streamer
    {
        virtual ~Streamer() = default;

        virtual bool FinalizeAssetLoading(AssetSlot& assetSlot) = 0;
        virtual bool IsFinalizeCompleted(AssetSlot& assetSlot) = 0;
    };


    struct DefaultStreamer final : public Streamer
    {
        bool FinalizeAssetLoading(AssetSlot& assetSlot) override;
        bool IsFinalizeCompleted(AssetSlot& assetSlot) override;
    };


    struct AssetManager final
    {
        static void Init();
        static void Shutdown();

        [[nodiscard]] static ResidencyTicket LoadAsset(AssetID assetId);

        [[nodiscard]] static AssetSlot* FindAssetSlot(AssetID assetId);

        template<class T>
        [[nodiscard]] static AssetLease<T> LoadAsset(Link<T> link)
        {
            const AssetID assetId = link.GetAssetID();
            const ResidencyTicket ticket = LoadAsset(assetId);
            AssetSlot* slot = FindAssetSlot(assetId);
            return AssetLease<T>(slot);
        }

        static void Tick();

    private:
        struct Impl;
        static Impl* GImpl;
    };
} // namespace FE::IO
