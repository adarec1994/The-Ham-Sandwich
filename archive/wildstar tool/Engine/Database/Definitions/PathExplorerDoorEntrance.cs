namespace ProjectWS.Engine.Database.Definitions
{
	public class PathExplorerDoorEntrance : TblRecord
	{
		public override string GetFileName() => "PathExplorerDoorEntrance";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint pathExplorerDoorTypeEnumSurface;
		public uint pathExplorerDoorTypeEnumMicro;
		public uint creature2IdSurface;
		public uint creature2IdMicro;
		public uint pathExplorerDoorId;
		public uint worldLocation2IdSurfaceRevealed;
	}
}
