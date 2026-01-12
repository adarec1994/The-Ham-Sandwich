namespace ProjectWS.Engine.Database.Definitions
{
	public class Creature2OutfitGroupEntry : TblRecord
	{
		public override string GetFileName() => "Creature2OutfitGroupEntry";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2OutfitGroupId;
		public uint creature2OutfitInfoId;
		public uint weight;
	}
}
