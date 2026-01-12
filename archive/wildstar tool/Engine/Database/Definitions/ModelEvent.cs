namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelEvent : TblRecord
	{
		public override string GetFileName() => "ModelEvent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string EnumName;
	}
}
