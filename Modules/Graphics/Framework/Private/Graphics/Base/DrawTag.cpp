#include <Graphics/Base/DrawTag.h>

namespace FE::Graphics
{
    uint32_t Internal::GetNextDrawTagValue()
    {
        static uint32_t value = 0;
        return value++;
    }
} // namespace FE::Graphics
