namespace ProjectWS.Engine.Database.Definitions
{
	public class PathScientistCreatureInfo : TblRecord
	{
		public override string GetFileName() => "PathScientistCreatureInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint scientistCreatureFlags;
		public string displayIcon;
		public uint prerequisiteIdScan;
		public uint prerequisiteIdRawScan;
		public uint prerequisiteIdScanCreature;
		public uint prerequisiteIdRawScanCreature;
		public uint spell4IdBuff00;
		public uint spell4IdBuff01;
		public uint spell4IdBuff02;
		public uint spell4IdBuff03;
		public uint checklistCount;
		public uint scientistCreatureTypeEnum;
		public uint lootId;
	}
}
