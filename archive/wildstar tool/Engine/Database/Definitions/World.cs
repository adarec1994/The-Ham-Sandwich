namespace ProjectWS.Engine.Database.Definitions
{
	public class World : TblRecord
	{
		public override string GetFileName() => "World";
		public override uint GetID() => this.ID;

		[Flags]
		public enum Flags : uint
		{
			unk0x1 = 0x1,
			unk0x2 = 0x2,
			unk0x4 = 0x4,
			unk0x8 = 0x8,
			unk0x10 = 0x10,
			unused0x20 = 0x20,
			unk0x40 = 0x40,
			unk0x80 = 0x80,
			unk0x100 = 0x100
		}

		public uint ID;
		public string? assetPath;
		public uint flags;
		public uint type;
		public string? screenPath;
		public string? screenModelPath;
		public uint chunkBounds00;
		public uint chunkBounds01;
		public uint chunkBounds02;
		public uint chunkBounds03;
		public uint plugAverageHeight;
		public uint localizedTextIdName;
		public uint minItemLevel;
		public uint maxItemLevel;
		public uint primeLevelOffset;
		public uint primeLevelMax;
		public uint veteranTierScalingType;
		public uint heroismMenaceLevel;
		public uint rewardRotationContentId;
    }
}
