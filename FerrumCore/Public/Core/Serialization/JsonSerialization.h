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
        bool BeginDocumentImpl(Rtti::TypeID expectedType, uint32_t version, uint64_t schemaHash) override;
        void EndDocumentImpl() override;
        bool BeginObjectImpl() override;
        void EndObjectImpl() override;
        bool BeginFieldImpl(festd::ascii_view name, uint64_t fieldID) override;
        void EndFieldImpl() override;
        bool BeginArrayImpl(uint32_t& size) override;
        void EndArrayImpl() override;
        bool BeginElementImpl(uint32_t index) override;
        void EndElementImpl() override;
        void StoreScalarImpl(ScalarKind kind, const void* value, uint32_t byteSize) override;
        void LoadScalarImpl(ScalarKind kind, void* value, uint32_t byteSize) override;
        void StoreBytesImpl(const void* value, uint32_t byteSize) override;
        void LoadBytesImpl(void* value, uint32_t byteSize) override;
        void StoreStringImpl(festd::string_view value) override;
        uint32_t LoadStringSizeImpl() override;
        void LoadStringImpl(festd::span<char> buffer) override;
        [[nodiscard]] uint64_t GetCurrentOffsetImpl() const override;
    };
} // namespace FE::Serialization
