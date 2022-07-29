namespace Ferrum.Core.Entities
{
    public interface IComponentSubsystem
    {
        void OnCreate();
        void OnUpdate();
        void OnDestroy();
    }
}
