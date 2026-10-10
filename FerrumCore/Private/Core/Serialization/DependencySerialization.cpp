#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/DependencySerialization.h>

namespace FE::Serialization
{
    namespace
    {
        // Serialize authored fields only to visit references; no redundant payload buffers are produced.
        struct DependencyOnlyFormat final : SerializationFormat
        {
            explicit DependencyOnlyFormat(Format format)
                : SerializationFormat(format)
            {
            }


            void ResetImpl() override {}


            ResultCode BeginStoreDocumentImpl(Rtti::TypeID, uint32_t, uint64_t) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode BeginLoadDocumentImpl(Rtti::TypeID, uint32_t, uint64_t) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode EndStoreDocumentImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode EndLoadDocumentImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode BeginStoreObjectImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode BeginLoadObjectImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode EndStoreObjectImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode EndLoadObjectImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode BeginStoreFieldImpl(festd::ascii_view, uint64_t) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode BeginLoadFieldImpl(festd::ascii_view, uint64_t, bool&) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode EndStoreFieldImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode EndLoadFieldImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode BeginStoreArrayImpl(uint32_t) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode BeginLoadArrayImpl(uint32_t&) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode EndStoreArrayImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode EndLoadArrayImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode BeginStoreElementImpl(uint32_t) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode BeginLoadElementImpl(uint32_t) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode EndStoreElementImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode EndLoadElementImpl() override
            {
                return ResultCode::kSuccess;
            }


            ResultCode StoreScalarImpl(Serialization::ScalarKind, const void*, uint32_t) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode LoadScalarImpl(Serialization::ScalarKind, void*, uint32_t) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode StoreBytesImpl(const void*, uint32_t) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode LoadBytesImpl(void*, uint32_t) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode StoreStringImpl(festd::string_view) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode LoadStringSizeImpl(uint32_t&) override
            {
                return ResultCode::kSuccess;
            }


            ResultCode LoadStringImpl(festd::span<char>) override
            {
                return ResultCode::kSuccess;
            }


            uint64_t GetStoreCurrentOffsetImpl() const override
            {
                return 0;
            }


            uint64_t GetLoadCurrentOffsetImpl() const override
            {
                return 0;
            }
        };
    } // namespace


    ResultCode VisitAssetReferences(const Rtti::Type& type, const void* value, void* userData,
                                    SerializationContext::AssetReferenceCallback callback, Format format)
    {
        FE_PROFILER_ZONE();
        FE_Assert(value && callback);
        if (!type.m_serialize)
            return ResultCode::kSuccess;

        IO::WriteOnlyMemoryStream stream;
        DependencyOnlyFormat visitor(format);
        SerializationContext context(&stream, visitor, userData, callback);
        return context.Store(type, value);
    }
} // namespace FE::Serialization
