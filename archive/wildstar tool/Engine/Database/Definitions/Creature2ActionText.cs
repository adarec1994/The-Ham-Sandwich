namespace ProjectWS.Engine.Database.Definitions
{
	public class Creature2ActionText : TblRecord
	{
		public override string GetFileName() => "Creature2ActionText";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdOnEnterCombat00;
		public uint localizedTextIdOnEnterCombat01;
		public uint localizedTextIdOnEnterCombat02;
		public uint localizedTextIdOnEnterCombat03;
		public float chanceToSayOnEnterCombat;
		public uint localizedTextIdOnDeath00;
		public uint localizedTextIdOnDeath01;
		public uint localizedTextIdOnDeath02;
		public uint localizedTextIdOnDeath03;
		public float chanceToSayOnDeath;
		public uint localizedTextIdOnKill00;
		public uint localizedTextIdOnKill01;
		public uint localizedTextIdOnKill02;
		public uint localizedTextIdOnKill03;
		public float chanceToSayOnKill;
	}
}
