namespace ProjectWS.Engine.Database.Definitions
{
	public class GenericUnlockSet : TblRecord
	{
		public override string GetFileName() => "GenericUnlockSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint genericUnlockScopeEnum;
		public uint genericUnlockEntryId00;
		public uint genericUnlockEntryId01;
		public uint genericUnlockEntryId02;
		public uint genericUnlockEntryId03;
		public uint genericUnlockEntryId04;
		public uint genericUnlockEntryId05;
	}
}
