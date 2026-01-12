namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemBudget : TblRecord
	{
		public override string GetFileName() => "ItemBudget";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float budget00;
		public float budget01;
		public float budget02;
		public float budget03;
		public float budget04;
	}
}
