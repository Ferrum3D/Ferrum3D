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


    void SceneImpl::DestroyView(View* view)
    {
        const auto found = festd::find_if(m_views.begin(), m_views.end(), [view](const Rc<View>& candidate) {
            return candidate.Get() == view;
        });
        FE_Assert(found != m_views.end());
        m_views.erase(found);
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
