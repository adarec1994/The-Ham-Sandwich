namespace ProjectWS.Engine.Database.Definitions
{
	public class PathLevel : TblRecord
	{
		public override string GetFileName() => "PathLevel";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint pathTypeEnum;
		public uint pathLevel;
		public uint pathXP;
	}
}
