namespace ProjectWS.Engine.Database.Definitions
{
	public class PrimalMatrixNode : TblRecord
	{
		public override string GetFileName() => "PrimalMatrixNode";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint positionX;
		public uint positionY;
		public uint primalMatrixNodeTypeEnum;
		public uint flags;
		public uint maxAllocations;
		public uint costRedEssence;
		public uint costBlueEssence;
		public uint costGreenEssence;
		public uint costPurpleEssence;
		public uint primalMatrixRewardIdWarrior;
		public uint primalMatrixRewardIdEngineer;
		public uint primalMatrixRewardIdEsper;
		public uint primalMatrixRewardIdMedic;
		public uint primalMatrixRewardIdStalker;
		public uint primalMatrixRewardIdSpellslinger;
	}
}
