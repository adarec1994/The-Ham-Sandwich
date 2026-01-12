namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldSky : TblRecord
	{
		public override string GetFileName() => "WorldSky";
		public override uint GetID() => this.ID;

		public uint ID;
		public string assetPath;
		public string assetPathInFlux;
		public uint color;

		public uint Id
		{
			get { return this.ID; }
			set { this.ID = value; }
		}

		public string AssetPath
		{
			get { return this.assetPath; }
			set { this.assetPath = value; }
		}

		public string ColorString
		{
			get 
			{
                return $"#{this.color:X}"; 
			}
		}
    }
}
