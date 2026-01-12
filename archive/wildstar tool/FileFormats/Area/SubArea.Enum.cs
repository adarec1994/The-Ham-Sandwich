namespace ProjectWS.FileFormats.Area
{
    public partial class SubArea
    {
        [System.Flags]
        public enum Flags : uint
        {
            hasHeightmap = 0x1,
            hasWorldLayerIDs = 0x2,
            hasBlendMap = 0x4,
            hasColorMap = 0x8,
            hasUnkMap = 0x10,
            unk0x20 = 0x20,
            hasSkyIDs = 0x40,
            hasSkyWeights = 0x80,
            hasShadowMap = 0x100,
            hasLoDHeightMap = 0x200,
            hasLoDHeightRange = 0x400,
            unk0x800 = 0x800,
            unk0x1000 = 0x1000,
            hascolorMapDXT = 0x2000,
            hasUnkMap0 = 0x4000,
            unk0x8000 = 0x8000,
            hasZoneBounds = 0x10000,
            hasBlendMapDXT = 0x20000,
            hasUnkMap1 = 0x40000,
            hasUnkMap2 = 0x80000,
            hasUnkMap3 = 0x100000,
            unk0x200000 = 0x200000,
            unk0x400000 = 0x400000,
            unk0x800000 = 0x800000,
            unk0x1000000 = 0x1000000,
            unk0x2000000 = 0x2000000,
            unk0x4000000 = 0x4000000,
            unk0x8000000 = 0x8000000,
            hasZoneIDs = 0x10000000,
            unk0x20000000 = 0x20000000,
            unk0x40000000 = 0x40000000,
            hasUnkMap4 = 0x80000000
        }

        public enum ChunkID : uint
        {
            PROP = 0x50524F50,
            curD = 0x63757244,
            WAtG = 0x57417447,
            wbsP = 0x77627350,
        }
    }
}
