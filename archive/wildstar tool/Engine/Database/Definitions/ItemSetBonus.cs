namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemSetBonus : TblRecord
	{
		public override string GetFileName() => "ItemSetBonus";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint requiredPower;
		public uint unitProperty2Id;
		public float scalar;
		public float offset;
		public uint spell4Id;
	}
}
