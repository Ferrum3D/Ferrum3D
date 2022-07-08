using System;
using System.Collections.Generic;
using System.Linq;

namespace Ferrum.Core.Framework
{
    public abstract class FrameworkBase : IFramework, IDisposable
    {
        public bool IsInitialized { get; private set; }
        private readonly HashSet<IFrameworkFactory> frameworkFactories = new();
        private readonly List<IFramework> frameworkDependencies = new();

        public void Initialize()
        {
            if (IsInitialized)
            {
                return;
            }

            GetFrameworkDependencies(frameworkFactories);
            foreach (var factory in frameworkFactories)
            {
                frameworkDependencies.Add(factory.Load());
            }

            IsInitialized = true;
        }

        public void UnloadDependencies()
        {
            if (!IsInitialized)
            {
                return;
            }

            foreach (var factory in frameworkFactories)
            {
                factory.Unload();
            }

            IsInitialized = false;
        }

        public virtual void Dispose()
        {
            UnloadDependencies();
            GC.SuppressFinalize(this);
        }

        protected TModule GetDependency<TModule>()
        {
            return frameworkDependencies.OfType<TModule>().FirstOrDefault();
        }

        protected virtual void GetFrameworkDependencies(ICollection<IFrameworkFactory> dependencies)
        {
        }
    }
}
