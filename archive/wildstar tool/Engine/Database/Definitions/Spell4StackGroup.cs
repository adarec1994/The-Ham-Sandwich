namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4StackGroup : TblRecord
	{
		public override string GetFileName() => "Spell4StackGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint stackCap;
		public uint stackTypeEnum;
	}
}
