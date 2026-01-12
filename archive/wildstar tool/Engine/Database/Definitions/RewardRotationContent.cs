namespace ProjectWS.Engine.Database.Definitions
{
	public class RewardRotationContent : TblRecord
	{
		public override string GetFileName() => "RewardRotationContent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint contentTypeEnum;
	}
}
