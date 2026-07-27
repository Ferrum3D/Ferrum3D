#pragma once
#include <Core/Serialization/Serialization.h>
#include <memory>

namespace FE::Serialization
{
    class PackedBinaryContext final : public SerializationContext
    {
    public:
        explicit PackedBinaryContext(IO::IStream* stream);
        ~PackedBinaryContext() override;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;

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
        void TransferScalar(ScalarKind kind, void* value, uint32_t byteSize) override;
        void TransferBytes(void* value, uint32_t byteSize) override;
        void TransferString(festd::string* output, festd::string_view input) override;
    };


    class TaggedBinaryContext final : public SerializationContext
    {
    public:
        explicit TaggedBinaryContext(IO::IStream* stream);
        ~TaggedBinaryContext() override;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;

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
        void TransferScalar(ScalarKind kind, void* value, uint32_t byteSize) override;
        void TransferBytes(void* value, uint32_t byteSize) override;
        void TransferString(festd::string* output, festd::string_view input) override;
    };
} // namespace FE::Serialization
