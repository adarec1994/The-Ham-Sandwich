namespace ProjectWS.Engine.Database.Definitions
{
	public class MatchingMapPrerequisite : TblRecord
	{
		public override string GetFileName() => "MatchingMapPrerequisite";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint matchingGameMapId;
		public uint matchingEligibilityFlagEnum;
	}
}
