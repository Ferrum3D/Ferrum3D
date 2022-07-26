namespace Ferrum.Core.Entities
{
    public abstract class ComponentSubsystem
    {
        protected internal ComponentSystem ParentSystem { get; internal set; }

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
