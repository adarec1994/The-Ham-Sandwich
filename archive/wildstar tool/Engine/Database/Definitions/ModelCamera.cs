namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelCamera : TblRecord
	{
		public override string GetFileName() => "ModelCamera";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string EnumName;
	}
}
