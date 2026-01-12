namespace ProjectWS.Engine.Database.Definitions
{
	public class CustomizationParameterMap : TblRecord
	{
		public override string GetFileName() => "CustomizationParameterMap";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint raceId;
		public uint genderEnum;
		public uint modelBoneId;
		public uint customizationParameterId;
		public uint dataOrder;
		public uint flags;
	}
}
