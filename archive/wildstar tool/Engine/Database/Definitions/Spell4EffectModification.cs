namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4EffectModification : TblRecord
	{
		public override string GetFileName() => "Spell4EffectModification";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint spell4EffectsId;
		public uint effectTypeEnum;
		public uint modificationParameterEnum;
		public uint priority;
		public uint modificationTypeEnum;
		public float data00;
		public float data01;
		public float data02;
		public float data03;
	}
}
