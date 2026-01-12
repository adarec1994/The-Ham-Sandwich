namespace ProjectWS.Engine.Database.Definitions
{
	public class GenericUnlockEntry : TblRecord
	{
		public override string GetFileName() => "GenericUnlockEntry";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdDescription;
		public string spriteIcon;
		public string spritePreview;
		public uint genericUnlockTypeEnum;
		public uint unlockObject;
	}
}
