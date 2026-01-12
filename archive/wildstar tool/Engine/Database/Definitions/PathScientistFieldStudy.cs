namespace ProjectWS.Engine.Database.Definitions
{
	public class PathScientistFieldStudy : TblRecord
	{
		public override string GetFileName() => "PathScientistFieldStudy";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2Id;
		public uint targetGroupId;
		public uint pathScientistFieldStudyFlags;
		public uint localizedTextIdChecklist00;
		public uint localizedTextIdChecklist01;
		public uint localizedTextIdChecklist02;
		public uint localizedTextIdChecklist03;
		public uint localizedTextIdChecklist04;
		public uint localizedTextIdChecklist05;
		public uint localizedTextIdChecklist06;
		public uint localizedTextIdChecklist07;
		public uint worldLocation2IdIndicator00;
		public uint worldLocation2IdIndicator01;
		public uint worldLocation2IdIndicator02;
		public uint worldLocation2IdIndicator03;
		public uint worldLocation2IdIndicator04;
		public uint worldLocation2IdIndicator05;
		public uint worldLocation2IdIndicator06;
		public uint worldLocation2IdIndicator07;
		public uint behaviorType00;
		public uint behaviorType01;
		public uint behaviorType02;
		public uint behaviorType03;
		public uint behaviorType04;
		public uint behaviorType05;
		public uint behaviorType06;
		public uint behaviorType07;
	}
}
