namespace ProjectWS.Engine.Database.Definitions
{
	public class DistanceDamageModifier : TblRecord
	{
		public override string GetFileName() => "DistanceDamageModifier";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float distancePercent00;
		public float distancePercent01;
		public float distancePercent02;
		public float distancePercent03;
		public float distancePercent04;
		public float damageModifier00;
		public float damageModifier01;
		public float damageModifier02;
		public float damageModifier03;
		public float damageModifier04;
	}
}
