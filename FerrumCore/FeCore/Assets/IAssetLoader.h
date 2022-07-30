#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/IO/IStream.h>
#include <FeCore/Math/Vector4.h>
#include <FeCore/Strings/String.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE::Assets
{
    enum class AssetMetadataType
    {
        String,
        Bool,
        Int,
        UInt,
        Float,
        Vector3,
        Vector4,
        UUID
    };

    // clang-format off
    template<AssetMetadataType T> struct CppTypeForAssetMetadataType          { using Type = void;     };
    template<> struct CppTypeForAssetMetadataType<AssetMetadataType::String>  { using Type = String;   };
    template<> struct CppTypeForAssetMetadataType<AssetMetadataType::Bool>    { using Type = bool;     };
    template<> struct CppTypeForAssetMetadataType<AssetMetadataType::Int>     { using Type = Int64;    };
    template<> struct CppTypeForAssetMetadataType<AssetMetadataType::UInt>    { using Type = UInt64;   };
    template<> struct CppTypeForAssetMetadataType<AssetMetadataType::Float>   { using Type = Float32;  };
    template<> struct CppTypeForAssetMetadataType<AssetMetadataType::Vector3> { using Type = Vector3F; };
    template<> struct CppTypeForAssetMetadataType<AssetMetadataType::Vector4> { using Type = Vector4F; };
    template<> struct CppTypeForAssetMetadataType<AssetMetadataType::UUID>    { using Type = UUID;     };
    // clang-format on

    class AssetMetadataField
    {
        FE_PUSH_MSVC_WARNING(4324)
        std::variant<Vector4F, Vector3F, String, UUID, Int64, UInt64, Float32, bool> m_Value;
        String m_Key;
        AssetMetadataType m_Type;
        bool m_Required;
        FE_POP_MSVC_WARNING

    public:
        template<AssetMetadataType T>
        inline static AssetMetadataField Create(const StringSlice& key,
                                                const typename CppTypeForAssetMetadataType<T>::Type& value, bool require = false)
        {
            AssetMetadataField result;
            result.m_Key      = key;
            result.m_Value    = value;
            result.m_Type     = T;
            result.m_Required = require;
            return result;
        }

        template<AssetMetadataType T>
        inline void SetValue(const typename CppTypeForAssetMetadataType<T>::Type& value)
        {
            FE_CORE_ASSERT(T == m_Type, "Invalid asset metadata type");
            m_Value = value;
        }

        [[nodiscard]] inline const String& GetKey() const
        {
            return m_Key;
        }

        [[nodiscard]] inline AssetMetadataType GetType() const
        {
            return m_Type;
        }

        template<AssetMetadataType T>
        [[nodiscard]] inline const typename CppTypeForAssetMetadataType<T>::Type& GetValue() const
        {
            FE_CORE_ASSERT(T == m_Type, "Invalid asset metadata type");
            return std::get<typename CppTypeForAssetMetadataType<T>::Type>(m_Value);
        }

        [[nodiscard]] inline bool IsRequired() const
        {
            return m_Required;
        }

        [[nodiscard]] inline bool IsOptional() const
        {
            return !m_Required;
        }
    };

    //! \brief Asset loader interface.
    //!
    //! Asset loaders are responsible for loading assets from streams.
    class IAssetLoader : public IObject
    {
    public:
        FE_CLASS_RTTI(IAssetLoader, "D0DE4F16-0C3C-44E9-9215-CBC6FC98EB22");

        ~IAssetLoader() override = default;

        //! \brief Get type of asset that this loader can load.
        [[nodiscard]] virtual AssetType GetAssetType() const = 0;

        //! \brief Create storage for asset.
        //!
        //! The storage will have one strong reference. Remove the reference when you are done with it.
        //! Asset<T>::Load() will take ownership of the storage and will remove this reference automatically.
        [[nodiscard]] virtual AssetStorage* CreateStorage() = 0;

        //! \brief Load asset from stream.
        //!
        //! Load asset from stream and store it in storage synchronously on this thread.
        //!
        //! \param [in] storage     - Storage to load asset into.
        //! \param [in] assetStream - Stream to load asset from.
        virtual void LoadAsset(AssetStorage* storage, IO::IStream* assetStream) = 0;

        //! \brief Get asset metadata fields that can be applied to this asset loader.
        //!
        //! \return A list of asset metadata fields.
        virtual ArraySlice<AssetMetadataField> GetAssetMetadataFields() = 0;

        //! \brief Load asset from raw data.
        //!
        //! The metadata is a list of key-value pairs loaded from *.meta.json files from project directory.
        //! The fields of metadata are defined by asset type specification. This function is used by the asset compiler
        //! to load raw files like png-images or fbx-meshes into an asset storage and then save them back to the disk
        //! in engine's format.
        //!
        //! \param [in] metadata    - Raw asset metadata.
        //! \param [in] assetStream - Stream to load asset from.
        virtual void LoadRawAsset(const List<AssetMetadataField>& metadata, AssetStorage* storage, IO::IStream* assetStream) = 0;

        //! \brief Save asset to stream.
        //!
        //! Write asset bytes from the storage in engine's internal format synchronously on this thread.
        //!
        //! \param [in] storage     - Storage containing asset to save.
        //! \param [in] assetStream - Stream to write asset to.
        virtual void SaveAsset(AssetStorage* storage, IO::IStream* assetStream) = 0;
    };
} // namespace FE::Assets
