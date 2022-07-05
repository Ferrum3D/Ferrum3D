#pragma once
#include <FeCore/Strings/StringSlice.h>
#include <FeCore/Utils/Result.h>

namespace FE::Osmium::Internal
{
    Result<UInt8*, StringSlice> LoadImageFromMemory(const UInt8* data, USize length, Int32& width, Int32& height);
    void FreeImageMemory(UInt8* data);
} // namespace FE::Osmium::Internal
