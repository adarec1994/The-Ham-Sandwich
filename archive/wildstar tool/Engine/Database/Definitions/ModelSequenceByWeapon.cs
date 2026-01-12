namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelSequenceByWeapon : TblRecord
	{
		public override string GetFileName() => "ModelSequenceByWeapon";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint modelSequenceId;
		public uint modelSequenceId1H;
		public uint modelSequenceId2H;
		public uint modelSequenceId2HL;
		public uint modelSequenceId2HGun;
		public uint modelSequenceIdPistols;
		public uint modelSequenceIdClaws;
		public uint modelSequenceIdShockPaddles;
		public uint modelSequenceIdEsper;
		public uint modelSequenceIdPsyblade;
		public uint modelSequenceIdHeavygun;
	}
}
