namespace ProjectWS.Engine.Database.Definitions
{
	public class VeteranTier : TblRecord
	{
		public override string GetFileName() => "VeteranTier";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint primeLevel;
		public uint veteranTierScalingType;
		public uint unitPropertyOverrideMenace;
	}
}
