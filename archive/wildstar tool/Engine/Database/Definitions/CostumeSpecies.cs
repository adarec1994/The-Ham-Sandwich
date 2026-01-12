namespace ProjectWS.Engine.Database.Definitions
{
	public class CostumeSpecies : TblRecord
	{
		public override string GetFileName() => "CostumeSpecies";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint componentLayoutId;
		public string enumName;
	}
}
