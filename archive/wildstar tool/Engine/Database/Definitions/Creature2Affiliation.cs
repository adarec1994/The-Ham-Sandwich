namespace ProjectWS.Engine.Database.Definitions
{
	public class Creature2Affiliation : TblRecord
	{
		public override string GetFileName() => "Creature2Affiliation";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdTitle;
		public uint miniMapMarkerId;
		public string overheadIconAssetPath;
	}
}
