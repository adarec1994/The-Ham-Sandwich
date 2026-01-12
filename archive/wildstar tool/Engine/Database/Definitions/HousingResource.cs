namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingResource : TblRecord
	{
		public override string GetFileName() => "HousingResource";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
	}
}
