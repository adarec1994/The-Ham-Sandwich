namespace ProjectWS.Engine.Database.Definitions
{
	public class SalvageException : TblRecord
	{
		public override string GetFileName() => "SalvageException";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint item2Id;
		public uint flags;
	}
}
