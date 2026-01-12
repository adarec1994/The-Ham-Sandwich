namespace ProjectWS.Engine.Database.Definitions
{
	public class MapZone : TblRecord
	{
		public override string GetFileName() => "MapZone";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;	// ID into LocalizedText.tbl ?
		public uint mapContinentId;			// ID into MapContinent.tbl
		public string folder;               // Path in "UI/Maps" folder that contains a rendered map
		public uint hexMinX;
		public uint hexMinY;
		public uint hexLimX;
		public uint hexLimY;
		public uint version;
		public uint mapZoneIdParent;		// ID into this table
		public uint worldZoneId;			// ID into WorldZone.tbl
		public uint flags;
		public uint prerequisiteIdVisibility;
		public uint rewardTrackId;
	}
}
