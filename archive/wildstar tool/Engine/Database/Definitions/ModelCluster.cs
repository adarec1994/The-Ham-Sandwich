namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelCluster : TblRecord
	{
		public override string GetFileName() => "ModelCluster";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string EnumName;
	}
}
