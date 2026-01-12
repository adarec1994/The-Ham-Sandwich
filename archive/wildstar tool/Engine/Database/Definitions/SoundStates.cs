namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundStates : TblRecord
	{
		public override string GetFileName() => "SoundStates";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint nameHash;
		public uint soundGroupHash;
	}
}
