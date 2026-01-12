namespace ProjectWS.Engine.Database.Definitions
{
	public class PrimalMatrixReward : TblRecord
	{
		public override string GetFileName() => "PrimalMatrixReward";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint primalMatrixRewardTypeEnum0;
		public uint primalMatrixRewardTypeEnum1;
		public uint primalMatrixRewardTypeEnum2;
		public uint primalMatrixRewardTypeEnum3;
		public uint objectId0;
		public uint objectId1;
		public uint objectId2;
		public uint objectId3;
		public uint subObjectId0;
		public uint subObjectId1;
		public uint subObjectId2;
		public uint subObjectId3;
		public float value0;
		public float value1;
		public float value2;
		public float value3;
	}
}
