#pragma once
#include <Core/Env/Environment.h>
#include <Core/Math/Vector2.h>
#include <Core/Math/Vector3UInt.h>
#include <Core/RTTI/ContainerShapes.h>
#include <Core/Strings/Format.h>

namespace FE
{
    struct BufferPointer final
    {
        Vector2UInt m_deviceAddress;

        BufferPointer() = default;

        explicit BufferPointer(const uint64_t deviceAddress)
            : m_deviceAddress(static_cast<uint32_t>(deviceAddress), static_cast<uint32_t>(deviceAddress >> 32))
        {
        }

        [[nodiscard]] uint64_t GetDeviceAddress() const
        {
            return static_cast<uint64_t>(m_deviceAddress.x) | (static_cast<uint64_t>(m_deviceAddress.y) << 32);
        }
    };

    static_assert(sizeof(BufferPointer) == sizeof(uint64_t));
    static_assert(alignof(BufferPointer) == alignof(uint32_t));
} // namespace FE


namespace FE::Graphics
{
    struct [[nodiscard]] SamplerDescriptor final : public TypedHandle<SamplerDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] TextureSRVDescriptor final : public TypedHandle<TextureSRVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] TextureUAVDescriptor final : public TypedHandle<TextureUAVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] BufferSRVDescriptor final : public TypedHandle<BufferSRVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] BufferUAVDescriptor final : public TypedHandle<BufferUAVDescriptor, uint32_t>
    {
    };

#define FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(name, baseDescriptor)                                                                    \
    template<class T>                                                                                                            \
    using name = baseDescriptor;

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture1DDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture2DDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture3DDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture1DArrayDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture2DArrayDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(TextureCubeDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(TextureCubeArrayDescriptor, TextureSRVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture1DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture2DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture3DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture1DArrayDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture2DArrayDescriptor, TextureUAVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture1DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture2DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture3DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture1DArrayDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture2DArrayDescriptor, TextureUAVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(BufferDescriptor, BufferSRVDescriptor);
    using ByteAddressBufferDescriptor = BufferSRVDescriptor;

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWBufferDescriptor, BufferUAVDescriptor);
    using RWByteAddressBufferDescriptor = BufferUAVDescriptor;

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentStructuredBufferDescriptor, BufferUAVDescriptor);
    using GloballyCoherentRWByteAddressBufferDescriptor = BufferUAVDescriptor;

#undef FE_FRAME_GRAPH_ALIAS_DESCRIPTOR


    //! Read-only view of a buffer of T, obtained from FrameGraph::GetSRV of a Core::StructuredBuffer<T>.
    template<class T>
    struct StructuredBufferDescriptor final
    {
        BufferSRVDescriptor m_descriptor;

        StructuredBufferDescriptor() = default;

        //! For views the type system cannot follow, e.g. a slice of a buffer.
        explicit StructuredBufferDescriptor(const BufferSRVDescriptor descriptor)
            : m_descriptor(descriptor)
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_descriptor.IsValid();
        }
    };


    //! Read-write view of a buffer of T, obtained from FrameGraph::GetUAV of a Core::StructuredBuffer<T>.
    template<class T>
    struct RWStructuredBufferDescriptor final
    {
        BufferUAVDescriptor m_descriptor;

        RWStructuredBufferDescriptor() = default;

        //! For views the type system cannot follow, e.g. a slice of a buffer.
        explicit RWStructuredBufferDescriptor(const BufferUAVDescriptor descriptor)
            : m_descriptor(descriptor)
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_descriptor.IsValid();
        }
    };

    template<class T>
    using GloballyCoherentRWStructuredBufferDescriptor = RWStructuredBufferDescriptor<T>;


    //! Constants of a type known only to the shaders that read them, e.g. the payload of a technique role.
    struct AnyConstantsPtr final
    {
        BufferPointer m_ptr{ UINT64_C(0) };

        [[nodiscard]] bool IsValid() const
        {
            return m_ptr.GetDeviceAddress() != 0;
        }

        FE_RTTI_Reflect("BD8F9053-88C1-4846-AC05-A0FA86026644");
    };


    //! Device address of immutable constants created with FrameGraph::CreateConstants, valid for one graph execution.
    //!
    //! It can be stored in push constants or in other constants. Passes that reference it, directly or through nested
    //! pointers, declare the accesses of every descriptor in the constants.
    template<class T>
    struct ConstantsPtr final
    {
        AnyConstantsPtr m_any;

        [[nodiscard]] bool IsValid() const
        {
            return m_any.IsValid();
        }

        [[nodiscard]] operator AnyConstantsPtr() const
        {
            return m_any;
        }
    };
} // namespace FE::Graphics


namespace FE::Rtti
{
    //! Traversal sees one untyped descriptor, so frame graphs recognize every instance by a single type.
    template<class T>
    struct Shape<Graphics::StructuredBufferDescriptor<T>>
    {
        static void Bind(Type& type)
        {
            static_assert(sizeof(Graphics::StructuredBufferDescriptor<T>) == sizeof(Graphics::BufferSRVDescriptor));
            Internal::BindFixedArrayShape<Graphics::StructuredBufferDescriptor<T>, Graphics::BufferSRVDescriptor, 1>(type);
        }
    };


    template<class T>
    struct Shape<Graphics::RWStructuredBufferDescriptor<T>>
    {
        static void Bind(Type& type)
        {
            static_assert(sizeof(Graphics::RWStructuredBufferDescriptor<T>) == sizeof(Graphics::BufferUAVDescriptor));
            Internal::BindFixedArrayShape<Graphics::RWStructuredBufferDescriptor<T>, Graphics::BufferUAVDescriptor, 1>(type);
        }
    };


    //! Traversal sees one untyped pointer, so frame graphs recognize every ConstantsPtr instance by a single type.
    template<class T>
    struct Shape<Graphics::ConstantsPtr<T>>
    {
        static void Bind(Type& type)
        {
            static_assert(sizeof(Graphics::ConstantsPtr<T>) == sizeof(Graphics::AnyConstantsPtr));
            Internal::BindFixedArrayShape<Graphics::ConstantsPtr<T>, Graphics::AnyConstantsPtr, 1>(type);
        }
    };
} // namespace FE::Rtti

FE_RTTI_Reflect(FE::Graphics::TextureSRVDescriptor, "4B5172EF-1E7A-43E2-B589-1BAC65428E92");
FE_RTTI_Reflect(FE::Graphics::TextureUAVDescriptor, "39287D51-E573-4428-AADF-74827CD4351E");
FE_RTTI_Reflect(FE::Graphics::BufferSRVDescriptor, "9FFDEB45-4643-43D9-859B-895C5E8012CF");
FE_RTTI_Reflect(FE::Graphics::BufferUAVDescriptor, "B03004C4-3108-46B1-A475-631CDC59BEDD");
FE_RTTI_Reflect(FE::Graphics::SamplerDescriptor, "0ADC65DE-1BA6-4A60-B320-333AD0C2F27B");


namespace FE::Graphics::Core
{
    enum class DescriptorType : uint32_t
    {
        kInvalid,
        kSampler,
        kSRV,
        kUAV,
    };


    enum class GraphicsApi : uint32_t
    {
        kNone,
        kVulkan,
        kCount,
    };


    enum class PassType : uint32_t
    {
        kGraphics,
        kCompute,
    };


    enum class DeviceQueueType : uint32_t
    {
        kGraphics, //!< Queue that supports graphics, compute, and copy operations.
        kCompute,  //!< Queue that supports compute and copy operations.
        kTransfer, //!< Queue that supports copy operations.

        kCount,
    };


    enum class ShaderSemanticName : uint32_t
    {
        kPosition,
        kColor,
        kNormal,
        kBinormal,
        kTangent,
        kBlendIndices,
        kBlendWeight,
        kTexCoord,
        kCount,
    };


    inline festd::string_view ToString(const ShaderSemanticName name)
    {
        switch (name)
        {
        case ShaderSemanticName::kPosition:
            return "POSITION";
        case ShaderSemanticName::kColor:
            return "COLOR";
        case ShaderSemanticName::kNormal:
            return "NORMAL";
        case ShaderSemanticName::kBinormal:
            return "BINORMAL";
        case ShaderSemanticName::kTangent:
            return "TANGENT";
        case ShaderSemanticName::kBlendIndices:
            return "BLENDINDICES";
        case ShaderSemanticName::kBlendWeight:
            return "BLENDWEIGHT";
        case ShaderSemanticName::kTexCoord:
            return "TEXCOORD";
        case ShaderSemanticName::kCount:
        default:
            FE_DebugBreak();
            return nullptr;
        }
    }


    struct ShaderSemantic final
    {
        ShaderSemanticName m_name : 4;
        uint32_t m_index : 4;

        [[nodiscard]] Env::Name ToName() const;

        static const ShaderSemantic kPosition;
        static const ShaderSemantic kColor;
        static const ShaderSemantic kNormal;
        static const ShaderSemantic kBinormal;
        static const ShaderSemantic kTangent;
        static const ShaderSemantic kBlendIndices;
        static const ShaderSemantic kBlendWeight;
        static const ShaderSemantic kTexCoord;
    };

    inline const ShaderSemantic ShaderSemantic::kPosition{ ShaderSemanticName::kPosition, 0 };
    inline const ShaderSemantic ShaderSemantic::kColor{ ShaderSemanticName::kColor, 0 };
    inline const ShaderSemantic ShaderSemantic::kNormal{ ShaderSemanticName::kNormal, 0 };
    inline const ShaderSemantic ShaderSemantic::kBinormal{ ShaderSemanticName::kBinormal, 0 };
    inline const ShaderSemantic ShaderSemantic::kTangent{ ShaderSemanticName::kTangent, 0 };
    inline const ShaderSemantic ShaderSemantic::kBlendIndices{ ShaderSemanticName::kBlendIndices, 0 };
    inline const ShaderSemantic ShaderSemantic::kBlendWeight{ ShaderSemanticName::kBlendWeight, 0 };
    inline const ShaderSemantic ShaderSemantic::kTexCoord{ ShaderSemanticName::kTexCoord, 0 };


    inline Env::Name ShaderSemantic::ToName() const
    {
        if (m_index == 0)
            return Env::Name{ Fmt::FixedFormatSized<32>("{}", ToString(m_name)) };
        return Env::Name{ Fmt::FixedFormatSized<32>("{}{}", ToString(m_name), m_index) };
    }


    inline uint32_t CalculateMipCount(const Vector3UInt size)
    {
        return 1 + Math::FloorLog2(Math::Max(size.x, Math::Max(size.y, size.z)));
    }


    inline uint32_t CalculateMipCount(const Vector2UInt size)
    {
        return 1 + Math::FloorLog2(Math::Max(size.x, size.y));
    }
} // namespace FE::Graphics::Core
