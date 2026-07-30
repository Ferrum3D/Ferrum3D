#pragma once
#include <Core/Serialization/Serialization.h>

namespace FE::Serialization
{
    struct JsonFormat final : public SerializationFormat
    {
        JsonFormat();
        ~JsonFormat() override;

    private:
        struct Impl;
        festd::unique_ptr<Impl> m_impl;

        void ResetImpl() override;
        ResultCode BeginStoreDocumentImpl(Rtti::TypeID expectedType, uint32_t version, uint64_t schemaHash) override;
        ResultCode BeginLoadDocumentImpl(Rtti::TypeID expectedType, uint32_t version, uint64_t schemaHash) override;
        ResultCode EndStoreDocumentImpl() override;
        ResultCode EndLoadDocumentImpl() override;
        ResultCode BeginStoreObjectImpl() override;
        ResultCode BeginLoadObjectImpl() override;
        ResultCode EndStoreObjectImpl() override;
        ResultCode EndLoadObjectImpl() override;
        ResultCode BeginStoreFieldImpl(festd::ascii_view name, uint64_t fieldID) override;
        ResultCode BeginLoadFieldImpl(festd::ascii_view name, uint64_t fieldID, bool& exists) override;
        ResultCode EndStoreFieldImpl() override;
        ResultCode EndLoadFieldImpl() override;
        ResultCode BeginStoreArrayImpl(uint32_t size) override;
        ResultCode BeginLoadArrayImpl(uint32_t& size) override;
        ResultCode EndStoreArrayImpl() override;
        ResultCode EndLoadArrayImpl() override;
        ResultCode BeginStoreElementImpl(uint32_t index) override;
        ResultCode BeginLoadElementImpl(uint32_t index) override;
        ResultCode EndStoreElementImpl() override;
        ResultCode EndLoadElementImpl() override;
        ResultCode StoreScalarImpl(ScalarKind kind, const void* value, uint32_t byteSize) override;
        ResultCode LoadScalarImpl(ScalarKind kind, void* value, uint32_t byteSize) override;
        ResultCode StoreBytesImpl(const void* value, uint32_t byteSize) override;
        ResultCode LoadBytesImpl(void* value, uint32_t byteSize) override;
        ResultCode StoreStringImpl(festd::string_view value) override;
        ResultCode LoadStringSizeImpl(uint32_t& size) override;
        ResultCode LoadStringImpl(festd::span<char> buffer) override;
        [[nodiscard]] uint64_t GetStoreCurrentOffsetImpl() const override;
        [[nodiscard]] uint64_t GetLoadCurrentOffsetImpl() const override;
    };
} // namespace FE::Serialization
