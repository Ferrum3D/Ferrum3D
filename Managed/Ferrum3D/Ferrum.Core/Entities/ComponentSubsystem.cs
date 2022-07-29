namespace Ferrum.Core.Entities
{
    public abstract class ComponentSubsystem<TParent> : IComponentSubsystem
        where TParent : ComponentSystem
    {
        protected internal TParent ParentSystem { get; internal set; }

        public virtual void OnCreate()
        {
        }

        public virtual void OnUpdate()
        {
        }

        public virtual void OnDestroy()
        {
        }
    }
}
