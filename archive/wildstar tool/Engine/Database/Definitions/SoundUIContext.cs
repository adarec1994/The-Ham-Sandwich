namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundUIContext : TblRecord
	{
		public override string GetFileName() => "SoundUIContext";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string luaVariableName;
		public uint soundEventId;
	}
}
