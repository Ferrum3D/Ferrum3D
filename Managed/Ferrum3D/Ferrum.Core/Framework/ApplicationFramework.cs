using System.Diagnostics;
using System.IO;
using Ferrum.Core.Assets;
using Ferrum.Core.Console;
using Ferrum.Core.Containers;
using Ferrum.Core.Entities;
using Ferrum.Core.EventBus;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Framework
{
    public abstract class ApplicationFramework : FrameworkBase
    {
        public static ApplicationFramework Instance { get; private set; }
        public Desc Descriptor { get; private set; }
        
        private uint frameCounter;
        private int exitCode;
        private bool stopRequested;

        private Engine engine;
        private ConsoleLogger logger;
        private readonly DisposableList<EventBusBase> eventBuses = new();
        private AssetManager assetManager;

        protected abstract bool CloseEventReceived { get; }

        protected virtual bool ShouldStop => CloseEventReceived || stopRequested;

        public virtual void Initialize(Desc desc)
        {
            Descriptor = desc;

            engine = new Engine();
            logger = new ConsoleLogger();

            eventBuses.Add(new FrameEventBus());

            if (desc.AssetDirectory != null)
            {
                assetManager = new AssetManager(Path.Combine(desc.AssetDirectory, "FerrumAssetIndex"));
            }

            Initialize();
            Instance = this;
        }

        public int RunMainLoop()
        {
            var sw = new Stopwatch();
            sw.Start();

            while (!ShouldStop)
            {
                BeginFrame();
                var eventArgs = new FrameEventArgs(frameCounter++, sw.ElapsedMilliseconds / 1000f);
                FrameEventBus.OnFrameStart(in eventArgs);
                FrameEventBus.OnUpdate(in eventArgs);
                Tick(eventArgs);
                FrameEventBus.OnLateUpdate(in eventArgs);
                FrameEventBus.OnFrameEnd(in eventArgs);
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
            assetManager?.Dispose();
            eventBuses.Dispose();
            logger?.Dispose();
            engine.Dispose();
            Instance = null;
        }

        protected abstract void BeginFrame();
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
