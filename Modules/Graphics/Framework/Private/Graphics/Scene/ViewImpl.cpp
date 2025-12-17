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
        m_moduleList.ForEachActive([&blackboard](IViewModule& module) {
            module.Update(blackboard);
        });
    }
} // namespace FE::Graphics
