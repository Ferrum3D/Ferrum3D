#pragma once
#include <Core/Serialization/Serialization.h>

namespace FE::Serialization
{
    class JsonContext final : public SerializationContext
    {
    public:
        explicit JsonContext(IO::IStream* stream);
        ~JsonContext() override;

    private:
        struct Impl;
        festd::unique_ptr<Impl> m_impl;

        void Reset() override;
        bool BeginDocument(Rtti::TypeID expectedType, uint32_t version, uint64_t schemaHash) override;
        void EndDocument() override;
        bool BeginObjectImpl() override;
        void EndObjectImpl() override;
        bool BeginField(festd::ascii_view name, uint64_t fieldID) override;
        void EndField() override;
        bool BeginArrayImpl(uint32_t& size) override;
        void EndArrayImpl() override;
        bool BeginElement(uint32_t index) override;
        void EndElement() override;
        void StoreScalarImpl(ScalarKind kind, const void* value, uint32_t byteSize) override;
        void LoadScalarImpl(ScalarKind kind, void* value, uint32_t byteSize) override;
        void StoreBytesImpl(const void* value, uint32_t byteSize) override;
        void LoadBytesImpl(void* value, uint32_t byteSize) override;
        void StoreStringImpl(festd::string_view value) override;
        uint32_t LoadStringSizeImpl() override;
        void LoadStringImpl(festd::span<char> buffer) override;
        [[nodiscard]] uint64_t GetCurrentOffset() const override;
    };
} // namespace FE::Serialization
