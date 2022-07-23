using System.Diagnostics;
using System.IO;
using Ferrum.Core.Assets;
using Ferrum.Core.Console;
using Ferrum.Core.Entities;
using Ferrum.Core.EventBus;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Framework
{
    public abstract class ApplicationFramework : FrameworkBase
    {
        public Desc Descriptor { get; private set; }
        private uint frameCounter;
        private int exitCode;
        private bool stopRequested;

        private Engine engine;
        private ConsoleLogger logger;
        private AssetManager assetManager;
        private World world;
        protected abstract bool CloseEventReceived { get; }

        protected virtual bool ShouldStop => CloseEventReceived || stopRequested;

        public virtual void Initialize(Desc desc)
        {
            Descriptor = desc;

            engine = new Engine();
            logger = new ConsoleLogger();

            if (desc.AssetDirectory != null)
            {
                assetManager = new AssetManager(Path.Combine(desc.AssetDirectory, "FerrumAssetIndex"));
            }

            world = new World();

            Initialize();
        }

        public int RunMainLoop()
        {
            var sw = new Stopwatch();
            sw.Start();

            while (!ShouldStop)
            {
                PollSystemEvents();
                Tick(new FrameEventArgs(frameCounter++, sw.ElapsedMilliseconds / 1000f));
                sw.Restart();
            }

            return exitCode;
        }

        public void Stop(int code)
        {
            exitCode = code;
            stopRequested = true;
        }

        public override void Dispose()
        {
            OnExit();
            base.Dispose();
            world?.Dispose();
            assetManager?.Dispose();
            logger?.Dispose();
            engine.Dispose();
        }

        protected abstract void PollSystemEvents();
        protected abstract void Tick(FrameEventArgs frameEventArgs);
        protected abstract void OnExit();

        public readonly struct Desc
        {
            public readonly string Name;
            public readonly string AssetDirectory;
            public readonly uint WindowWidth;
            public readonly uint WindowHeight;
            public readonly bool Fullscreen;

            public Desc(string name, string assetDirectory = null, uint windowWidth = 800, uint windowHeight = 600,
                bool fullscreen = false)
            {
                Name = name;
                AssetDirectory = assetDirectory;
                WindowWidth = windowWidth;
                WindowHeight = windowHeight;
                Fullscreen = fullscreen;
            }
        }
    }
}
