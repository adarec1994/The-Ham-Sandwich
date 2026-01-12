namespace ProjectWS.Engine.Database.Definitions
{
	public class LevelUpUnlock : TblRecord
	{
		public override string GetFileName() => "LevelUpUnlock";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint levelUpUnlockSystemEnum;
		public uint level;
		public uint levelUpUnlockTypeId;
		public uint localizedTextIdDescription;
		public string displayIcon;
		public uint prerequisiteId;
		public uint levelUpUnlockValue;
	}
}
