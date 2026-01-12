namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Modification : TblRecord
	{
		public override string GetFileName() => "Spell4Modification";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint spell4EffectsId;
		public uint modificationParameterEnum;
		public uint priority;
		public uint modificationTypeEnum;
		public float data00;
		public float data01;
		public float data02;
		public float data03;
	}
}
