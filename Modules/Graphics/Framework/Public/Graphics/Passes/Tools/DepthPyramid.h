#pragma once
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Tools::DepthPyramid
{
    //! One reduction of a reverse-Z depth target.
    //!
    //! The depth target is treated as padded to powers of two per axis by repeating its edge texels. Mip zero has half the
    //! padded size, so every texel reduces exactly 2x2 texels of the level above.
    struct Output final
    {
        Rc<Core::Texture> m_texture;
        Vector2UInt m_size = Vector2UInt::kZero;
        //! Converts depth target UVs to pyramid UVs: the depth target covers this fraction of the padded area.
        Vector2 m_uvScale = Vector2(1.0f);
        uint32_t m_mipCount = 0;
    };


    //! The farthest pyramid stores the smallest reverse-Z depth of each texel, which is conservative for occlusion culling.
    //! The closest pyramid stores the largest depth.
    struct Pyramids final
    {
        Output m_farthest;
        Output m_closest;
    };


    struct Settings final
    {
        bool m_buildsClosest = true;
        //! A persistent pyramid keeps its memory across frames, e.g. to test against a frozen camera; otherwise it only lives
        //! until the graph executes.
        bool m_isPersistent = false;
    };


    //! Gather 2x2 depth texels per thread and reduce them in LDS to the first mips of both pyramids, then reduce the rest of
    //! each pyramid with a single-pass downsampler.
    Pyramids AddPass(Core::FrameGraph& graph, Core::Texture* depth, const Settings& settings = {});
} // namespace FE::Graphics::Tools::DepthPyramid
