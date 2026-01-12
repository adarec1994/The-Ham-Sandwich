namespace ProjectWS.Engine.Database.Definitions
{
	public class Quest2Difficulty : TblRecord
	{
		public override string GetFileName() => "Quest2Difficulty";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float xpMultiplier;
		public float cashRewardMultiplier;
		public float repRewardMultiplier;
	}
}
