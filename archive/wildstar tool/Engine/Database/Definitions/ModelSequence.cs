namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelSequence : TblRecord
	{
		public override string GetFileName() => "ModelSequence";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public uint FallBackID;
		public uint flag;
	}
}
