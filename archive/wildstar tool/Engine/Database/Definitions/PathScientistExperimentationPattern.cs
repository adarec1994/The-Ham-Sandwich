namespace ProjectWS.Engine.Database.Definitions
{
	public class PathScientistExperimentationPattern : TblRecord
	{
		public override string GetFileName() => "PathScientistExperimentationPattern";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint localizedTextIdDescription;
		public uint pathMissionId;
		public uint pathScientistExperimentationId;
		public string iconAssetPath;
	}
}
