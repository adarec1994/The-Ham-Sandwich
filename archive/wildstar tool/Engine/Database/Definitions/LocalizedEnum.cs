namespace ProjectWS.Engine.Database.Definitions
{
	public class LocalizedEnum : TblRecord
	{
		public override string GetFileName() => "LocalizedEnum";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string enumName;
		public uint localizedTextId;
	}
}
