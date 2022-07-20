using System.Runtime.InteropServices;

namespace Ferrum.Core.Entities
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct Entity
    {
        public static readonly Entity Null = new(uint.MaxValue);

        public uint Id => id & EntityIdMask;
        public uint Version => id >> EntityIdBitCount;

        private readonly uint id;

        // These constants must be the same as in C++ header "FerrumCore/FeCore/ECS/ECSCommon.h"
        private const int VersionBitCount = 10;
        private const int EntityIdBitCount = 32 - VersionBitCount;
        private const uint EntityIdMask = (1 << EntityIdBitCount) - 1;

        private Entity(uint id)
        {
            this.id = id;
        }
    }
}
