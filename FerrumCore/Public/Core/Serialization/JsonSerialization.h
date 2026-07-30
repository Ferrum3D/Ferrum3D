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
        ResultCode BeginDocumentImpl(Rtti::TypeID expectedType, uint32_t version, uint64_t schemaHash) override;
        ResultCode EndDocumentImpl() override;
        ResultCode BeginObjectImpl() override;
        ResultCode EndObjectImpl() override;
        ResultCode BeginFieldImpl(festd::ascii_view name, uint64_t fieldID, bool& exists) override;
        ResultCode EndFieldImpl() override;
        ResultCode BeginArrayImpl(uint32_t& size) override;
        ResultCode EndArrayImpl() override;
        ResultCode BeginElementImpl(uint32_t index) override;
        ResultCode EndElementImpl() override;
        ResultCode StoreScalarImpl(ScalarKind kind, const void* value, uint32_t byteSize) override;
        ResultCode LoadScalarImpl(ScalarKind kind, void* value, uint32_t byteSize) override;
        ResultCode StoreBytesImpl(const void* value, uint32_t byteSize) override;
        ResultCode LoadBytesImpl(void* value, uint32_t byteSize) override;
        ResultCode StoreStringImpl(festd::string_view value) override;
        ResultCode LoadStringSizeImpl(uint32_t& size) override;
        ResultCode LoadStringImpl(festd::span<char> buffer) override;
        [[nodiscard]] uint64_t GetCurrentOffsetImpl() const override;
    };
} // namespace FE::Serialization
