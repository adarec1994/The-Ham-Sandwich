namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingPlotType : TblRecord
	{
		public override string GetFileName() => "HousingPlotType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint maxPlacedDecor;
	}
}
