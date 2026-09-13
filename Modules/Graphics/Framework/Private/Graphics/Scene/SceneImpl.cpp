#include <Graphics/Scene/SceneImpl.h>
#include <Graphics/Scene/ViewImpl.h>

namespace FE::Graphics
{
    SceneImpl::SceneImpl(Renderer* renderer)
        : Scene(renderer)
    {
    }


    SceneImpl::~SceneImpl() {}


    View* SceneImpl::CreateView()
    {
        Rc<View> view = Memory::DefaultNew<ViewImpl>(this);
        m_views.push_back(view);
        return view.Get();
    }


    uint32_t SceneImpl::GetViewCount() const
    {
        return m_views.size();
    }


    View* SceneImpl::GetView(const uint32_t index) const
    {
        return m_views[index].Get();
    }

} // namespace FE::Graphics
