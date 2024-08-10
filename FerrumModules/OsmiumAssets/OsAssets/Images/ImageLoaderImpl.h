#pragma once
#include <FeCore/Strings/StringSlice.h>
#include <FeCore/Utils/Result.h>
#include <FeCore/IO/IStream.h>

namespace FE::Osmium::Internal
{
    Result<uint8_t*, StringSlice> LoadImageFromMemory(const uint8_t* data, size_t length, int32_t& width, int32_t& height, int32_t& channels);
    void WriteImageToStream(const uint8_t* data, int32_t width, int32_t height, IO::IStream* stream);
    void FreeImageMemory(uint8_t* data);
} // namespace FE::Osmium::Internal
