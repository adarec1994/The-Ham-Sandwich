namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillSchematic2 : TblRecord
	{
		public override string GetFileName() => "TradeskillSchematic2";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint tradeSkillId;
		public uint item2IdOutput;
		public uint item2IdOutputFail;
		public uint outputCount;
		public uint lootId;
		public uint tier;
		public uint flags;
		public uint item2IdMaterial00;
		public uint item2IdMaterial01;
		public uint item2IdMaterial02;
		public uint item2IdMaterial03;
		public uint item2IdMaterial04;
		public uint materialCost00;
		public uint materialCost01;
		public uint materialCost02;
		public uint materialCost03;
		public uint materialCost04;
		public uint tradeskillSchematic2IdParent;
		public float vectorX;
		public float vectorY;
		public float radius;
		public float critRadius;
		public uint item2IdOutputCrit;
		public uint outputCountCritBonus;
		public uint priority;
		public uint maxAdditives;
		public uint discoverableQuadrant;
		public float discoverableRadius;
		public float discoverableAngle;
		public uint tradeskillCatalystOrderingId;
	}
}
