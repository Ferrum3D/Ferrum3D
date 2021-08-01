#include <FeCore/Console/FeLog.h>
#include <FeCore/Memory/Memory.h>

class LargeClass
{
public:
    LargeClass()
    {
        FE_LOG_MESSAGE("Allocating LargeClass {}", sizeof(*this));
        for (char& c : Data)
            c = 0;
    }

    ~LargeClass()
    {
        FE_LOG_MESSAGE("Deallocating LargeClass");
    }

    char Data[256 * 1024 * 1024];
};

int main()
{
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    auto logger    = FE::MakeShared<FE::Debug::ConsoleLogger>();
    auto large     = FE::MakeShared<LargeClass>();
    FE::String str = "loooooooooooooooooooooooooong";
    for (int i = 0; i < 4; ++i)
    {
        FE_LOG_MESSAGE("{}", FE::GlobalAllocator<FE::HeapAllocator>::Get().TotalAllocated());
        FE_LOG_MESSAGE("{}", str.Capacity());
        FE_LOG_WARNING("{}", FE::GlobalAllocator<FE::HeapAllocator>::Get().TotalAllocated());
        FE_LOG_WARNING("{}", str.Capacity());
        FE_LOG_ERROR("{}", FE::GlobalAllocator<FE::HeapAllocator>::Get().TotalAllocated());
        FE_LOG_ERROR("{}", str.Capacity());
    }
}
