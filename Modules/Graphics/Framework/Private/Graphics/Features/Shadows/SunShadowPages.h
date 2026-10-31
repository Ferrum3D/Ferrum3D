#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::SunShadow
{
    //! Frame-local page table and atlas of the clip levels of one camera view.
    struct PageResources final
    {
        Rc<Core::Buffer> m_pageTable;
        Rc<Core::Buffer> m_counters;
        Rc<Core::Texture> m_atlas;
        uint32_t m_pageCount = 0;

        [[nodiscard]] static PageResources Create(Core::Device* device, uint32_t levelCount);
    };


    struct PageInputs final
    {
        //! A structured buffer with one SunShadowData.
        Core::Buffer* m_data = nullptr;
        //! A structured buffer with the ViewData of the camera whose depth requests pages.
        Core::Buffer* m_view = nullptr;
        Core::Texture* m_depth = nullptr;
    };


    //! Clear the page table, request the pages the camera depth needs, back them with atlas pages and clear those.
    void AddPagePasses(Core::FrameGraph& graph, const PageInputs& inputs, const PageResources& resources);
} // namespace FE::Graphics::SunShadow
