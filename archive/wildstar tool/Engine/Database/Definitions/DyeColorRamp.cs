namespace ProjectWS.Engine.Database.Definitions
{
	public class DyeColorRamp : TblRecord
	{
		public override string GetFileName() => "DyeColorRamp";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public uint localizedTextIdName;
		public uint rampIndex;
		public float costMultiplier;
		public uint componentMapEnum;
		public uint prerequisiteId;
	}
}
