namespace ProjectWS.Engine.Database.Definitions
{
	public class AttributeMilestoneGroup : TblRecord
	{
		public override string GetFileName() => "AttributeMilestoneGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint requiredAttributeAmount00;
		public uint requiredAttributeAmount01;
		public uint requiredAttributeAmount02;
		public uint requiredAttributeAmount03;
		public uint requiredAttributeAmount04;
		public uint requiredAttributeAmount05;
		public uint requiredAttributeAmount06;
		public uint requiredAttributeAmount07;
		public uint requiredAttributeAmount08;
		public uint requiredAttributeAmount09;
	}
}
