namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemDisplaySourceEntry : TblRecord
	{
		public override string GetFileName() => "ItemDisplaySourceEntry";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint itemSourceId;
		public uint item2TypeId;
		public uint itemMinLevel;
		public uint itemMaxLevel;
		public uint itemDisplayId;
		public string icon;
	}
}
