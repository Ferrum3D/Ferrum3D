namespace Ferrum.Core.Framework
{
    public interface IFrameworkFactory
    {
        IFramework Load();
        void Unload();
    }
}
