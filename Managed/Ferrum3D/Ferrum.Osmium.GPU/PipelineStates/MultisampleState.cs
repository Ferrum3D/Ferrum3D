using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.PipelineStates
{
    public readonly struct MultisampleState
    {
        public static readonly MultisampleState None = new MultisampleState(1, 1.0f, false);
        public static readonly MultisampleState X4 = new MultisampleState(4, 0.2f, true);
        public readonly int SampleCount;
        public readonly float MinSampleShading;
        public readonly bool SampleShadingEnabled;

        private MultisampleState(int sampleCount, float minSampleShading, bool sampleShadingEnabled)
        {
            SampleCount = sampleCount;
            MinSampleShading = minSampleShading;
            SampleShadingEnabled = sampleShadingEnabled;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct Native
        {
            public readonly int SampleCount;
            public readonly float MinSampleShading;
            public readonly int SampleShadingEnabled;

            public Native(MultisampleState state)
            {
                SampleCount = state.SampleCount;
                MinSampleShading = state.MinSampleShading;
                SampleShadingEnabled = state.SampleShadingEnabled ? 1 : 0;
            }
        }
    }
}
