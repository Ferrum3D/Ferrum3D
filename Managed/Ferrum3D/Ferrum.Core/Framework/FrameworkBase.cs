using System;
using System.Collections.Generic;

namespace Ferrum.Core.Framework
{
    public abstract class FrameworkBase : IFramework, IDisposable
    {
        public bool IsInitialized { get; private set; }
        private readonly List<IFrameworkFactory> frameworkDependencies = new();

        public void Initialize()
        {
            if (IsInitialized)
            {
                return;
            }

            GetFrameworkDependencies(frameworkDependencies);
            for (var i = 0; i < frameworkDependencies.Count; ++i)
            {
                frameworkDependencies[i].Load();
            }

            IsInitialized = true;
        }

        public void UnloadDependencies()
        {
            if (!IsInitialized)
            {
                return;
            }

            for (var i = 0; i < frameworkDependencies.Count; ++i)
            {
                frameworkDependencies[i].Unload();
            }

            IsInitialized = false;
        }

        public virtual void Dispose()
        {
            UnloadDependencies();
        }

        protected virtual void GetFrameworkDependencies(List<IFrameworkFactory> dependencies)
        {
        }
    }
}
