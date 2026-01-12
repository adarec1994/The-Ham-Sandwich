namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingDecorType : TblRecord
	{
		public override string GetFileName() => "HousingDecorType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public string luaString;
	}
}
