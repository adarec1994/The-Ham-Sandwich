namespace ProjectWS.Engine.Database.Definitions
{
	public class PositionalRequirement : TblRecord
	{
		public override string GetFileName() => "PositionalRequirement";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint angleCenter;
		public uint angleRange;
		public uint flags;
	}
}
