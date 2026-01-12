namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingMannequinPose : TblRecord
	{
		public override string GetFileName() => "HousingMannequinPose";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string enumName;
		public uint localizedTextId;
		public uint modelSequenceId;
	}
}
