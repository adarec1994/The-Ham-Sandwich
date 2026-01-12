namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundSwitch : TblRecord
	{
		public override string GetFileName() => "SoundSwitch";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint nameHash;
		public uint groupHash;
	}
}
