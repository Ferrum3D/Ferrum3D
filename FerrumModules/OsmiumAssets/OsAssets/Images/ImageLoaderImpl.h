#pragma once
#include <FeCore/Strings/StringSlice.h>
#include <FeCore/Utils/Result.h>
#include <FeCore/IO/IStream.h>

namespace FE::Osmium::Internal
{
    Result<UInt8*, StringSlice> LoadImageFromMemory(const UInt8* data, USize length, Int32& width, Int32& height, Int32& channels);
    void WriteImageToStream(const UInt8* data, Int32 width, Int32 height, IO::IStream* stream);
    void FreeImageMemory(UInt8* data);
} // namespace FE::Osmium::Internal
