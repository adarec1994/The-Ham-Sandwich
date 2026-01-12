namespace ProjectWS.Engine.Database.Definitions
{
	public class TargetMarker : TblRecord
	{
		public override string GetFileName() => "TargetMarker";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint visualEffectId;
		public string iconPath;
	}
}
