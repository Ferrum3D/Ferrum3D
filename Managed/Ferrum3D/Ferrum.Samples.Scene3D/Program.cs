﻿using Ferrum.Core.EventBus;
using Ferrum.Core.Framework;
using Ferrum.Osmium;
using Ferrum.Osmium.FrameGraph.FrameGraph;

namespace Ferrum.Samples.Scene3D
{
    internal class ExampleApplication : OsmiumApplication
    {
        public const string ApplicationName = "Ferrum3D - 3D Scene";
        private FrameGraph frameGraph;

        public override void Initialize(Desc desc)
        {
            base.Initialize(desc);
            frameGraph = new FrameGraph();
        }

        protected override void Tick(FrameEventArgs frameEventArgs)
        {
        }

        protected override void OnExit()
        {
            frameGraph.Dispose();
            base.OnExit();
        }
    }

    internal static class Program
    {
        private static int Main()
        {
            using var app = new ExampleApplication();
            app.Initialize(new ApplicationFramework.Desc(ExampleApplication.ApplicationName));
            return app.RunMainLoop();
        }
    }
}