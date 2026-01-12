using ProjectWS.FileFormats.Area;

namespace ProjectWS.Editor.Tools
{
    public partial class PropTool
    {
        public class PlacementProperty
        {
            public ushort minX { get; }
            public ushort minY { get; }
            public ushort maxX { get; }
            public ushort maxY { get; }
            public ushort size { get; }

            public PlacementProperty(Placement placement)
            {
                this.minX = placement.minX;
                this.minY = placement.minY;
                this.maxX = placement.maxX;
                this.maxY = placement.maxY;
                this.size = placement.size;
            }
        }
    }
}
