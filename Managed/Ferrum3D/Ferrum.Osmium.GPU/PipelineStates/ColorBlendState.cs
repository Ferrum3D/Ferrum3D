using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.Math;

namespace Ferrum.Osmium.GPU.PipelineStates
{
    public readonly struct ColorBlendState
    {
        public readonly List<TargetColorBlending> TargetBlendStates;
        public readonly Vector4F BlendConstants;

        public ColorBlendState(Vector4F blendConstants, params TargetColorBlending[] targetBlendStates)
        {
            TargetBlendStates = targetBlendStates.ToList();
            BlendConstants = blendConstants;
        }

        public ColorBlendState(params TargetColorBlending[] targetBlendStates)
            : this(Vector4F.Zero, targetBlendStates)
        {
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public readonly float BlendConstantX;
            public readonly float BlendConstantY;
            public readonly float BlendConstantZ;
            public readonly float BlendConstantW;
            public readonly ByteBuffer.Native TargetBlendStates;

            public Native(ColorBlendState state)
            {
                BlendConstantX = state.BlendConstants.X;
                BlendConstantY = state.BlendConstants.Y;
                BlendConstantZ = state.BlendConstants.Z;
                BlendConstantW = state.BlendConstants.W;
                TargetBlendStates = new NativeArray<TargetColorBlending>(state.TargetBlendStates).Detach();
            }
        }
    }
}
