namespace ProjectWS.Engine.Database.Definitions
{
	public class ClassSecondaryStatBonus : TblRecord
	{
		public override string GetFileName() => "ClassSecondaryStatBonus";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint unitProperty2IdSecondaryStat00;
		public uint unitProperty2IdSecondaryStat01;
		public uint unitProperty2IdSecondaryStat02;
		public float modifier00;
		public float modifier01;
		public float modifier02;
	}
}
