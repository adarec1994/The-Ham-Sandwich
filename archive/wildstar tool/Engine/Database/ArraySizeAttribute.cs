using System;

namespace ProjectWS.Engine.Database
{
    [AttributeUsage(AttributeTargets.Field)]
    public sealed class ArraySizeAttribute : Attribute
    {
        public int Size { get; private set; }

        public ArraySizeAttribute(int size)
        {
            Size = size;
        }
    }
}