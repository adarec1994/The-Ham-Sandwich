namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingWarplotBossToken : TblRecord
	{
		public override string GetFileName() => "HousingWarplotBossToken";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint spell4IdSummon;
		public uint minimumUpgradeTierEnum;
		public uint housingPlugItemIdLinked;
	}
}
