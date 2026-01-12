namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelAttachment : TblRecord
	{
		public override string GetFileName() => "ModelAttachment";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string EnumName;
	}
}
