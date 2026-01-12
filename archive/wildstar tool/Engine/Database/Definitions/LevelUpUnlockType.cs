namespace ProjectWS.Engine.Database.Definitions
{
	public class LevelUpUnlockType : TblRecord
	{
		public override string GetFileName() => "LevelUpUnlockType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
