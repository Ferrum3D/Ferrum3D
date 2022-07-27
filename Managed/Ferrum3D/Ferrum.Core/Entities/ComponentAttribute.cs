using System;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Entities
{
    [AttributeUsage(AttributeTargets.Struct)]
    public class ComponentAttribute : Attribute
    {
        public Uuid Type { get; }
        public uint Alignment { get; set; }
        public bool Unmanaged { get; set; }

        public ComponentAttribute(string typeUuid)
        {
            Type = Uuid.Parse(typeUuid);
        }
    }
}
