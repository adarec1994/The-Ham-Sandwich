namespace ProjectWS.Engine.Database.Definitions
{
	public class Entitlement : TblRecord
	{
		public override string GetFileName() => "Entitlement";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint maxCount;
		public uint flags;
		public uint spell4IdPersistentBuff;
		public uint characterTitleId;
		public uint localizedTextIdName;
		public uint localizedTextIdDescription;
		public string buttonIcon;
	}
}
