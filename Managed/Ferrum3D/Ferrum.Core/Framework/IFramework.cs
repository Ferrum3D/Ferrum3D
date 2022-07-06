namespace Ferrum.Core.Framework
{
    public interface IFramework
    {
        bool IsInitialized { get; }

        void Initialize();
        void UnloadDependencies();
    }
}
