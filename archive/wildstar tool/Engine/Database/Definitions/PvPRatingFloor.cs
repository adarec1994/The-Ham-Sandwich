namespace ProjectWS.Engine.Database.Definitions
{
	public class PvPRatingFloor : TblRecord
	{
		public override string GetFileName() => "PvPRatingFloor";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public uint pvpRatingTypeEnum;
		public uint floorValue;
		public uint localizedTextIdLabel;
	}
}
