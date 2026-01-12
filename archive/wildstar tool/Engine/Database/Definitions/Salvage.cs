namespace ProjectWS.Engine.Database.Definitions
{
	public class Salvage : TblRecord
	{
		public override string GetFileName() => "Salvage";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint item2TypeId;
		public uint level;
	}
}
