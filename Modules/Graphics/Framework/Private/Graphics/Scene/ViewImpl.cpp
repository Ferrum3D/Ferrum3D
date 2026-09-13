#include <Graphics/Scene/ViewImpl.h>

namespace FE::Graphics
{
    ViewImpl::ViewImpl(Scene* scene)
        : View(scene)
    {
    }


    ViewImpl::~ViewImpl() = default;


    void ViewImpl::Update(Core::FrameGraphBlackboard& blackboard)
    {
        m_moduleList.ForEachActive([&blackboard](ViewModuleBase& module) {
            module.Update(blackboard);
        });
    }


    void ViewImpl::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Graphics
