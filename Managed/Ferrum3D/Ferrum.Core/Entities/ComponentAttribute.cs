using System;

namespace Ferrum.Core.Entities
{
    [AttributeUsage(AttributeTargets.Struct)]
    public class ComponentAttribute : Attribute
    {
        public Guid Type { get; }
        public uint Alignment { get; set; } = 4;
        public bool Unmanaged { get; set; }

        public ComponentAttribute(string typeId)
        {
            Type = Guid.Parse(typeId);
        }
    }
}
