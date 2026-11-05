#include <Shaders/Tests/ConstantsProbe.h>

[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(1, 1, 1)
void main()
{
    const Outer outer = GConstants.m_outer.Load();
    Inner inner = outer.m_inner.Load();
    inner.m_output.Store(0, inner.m_value * outer.m_multiplier);

    Inner erased = GConstants.m_erased.As<Inner>().Load();
    erased.m_output.Store(1, erased.m_value);
}
