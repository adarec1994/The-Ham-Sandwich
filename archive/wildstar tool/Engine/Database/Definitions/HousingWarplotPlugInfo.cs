namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingWarplotPlugInfo : TblRecord
	{
		public override string GetFileName() => "HousingWarplotPlugInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint housingPlugItemId;
		public uint maintenanceCost;
		public uint upgradeCost00;
		public uint upgradeCost01;
		public uint upgradeCost02;
		public uint spell4IdAbility00;
		public uint spell4IdAbility01;
		public uint spell4IdAbility02;
		public uint spell4IdAbility03;
		public uint spell4IdAbility04;
		public uint spell4IdAbility05;
		public uint spell4IdAbility06;
		public uint spell4IdAbility07;
		public uint spell4IdAbility08;
		public uint spell4IdAbility09;
		public uint spell4IdAbility10;
		public uint spell4IdAbility11;
	}
}
