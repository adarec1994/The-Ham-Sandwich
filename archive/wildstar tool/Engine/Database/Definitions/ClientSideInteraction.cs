namespace ProjectWS.Engine.Database.Definitions
{
	public class ClientSideInteraction : TblRecord
	{
		public override string GetFileName() => "ClientSideInteraction";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint interactionType;
		public uint threshold;
		public uint duration;
		public uint incrementValue;
		public uint windowSize;
		public uint decay;
		public uint flags;
		public uint tapTime0;
		public uint tapTime1;
		public uint visualEffectId0;
		public uint visualEffectId1;
		public uint visualEffectId2;
		public uint visualEffectId3;
		public uint visualEffectIdTarget00;
		public uint visualEffectIdTarget01;
		public uint visualEffectIdTarget02;
		public uint visualEffectIdTarget03;
		public uint visualEffectIdTarget04;
		public uint visualEffectIdCaster00;
		public uint visualEffectIdCaster01;
		public uint visualEffectIdCaster02;
		public uint visualEffectIdCaster03;
		public uint visualEffectIdCaster04;
		public uint localizedTextIdContext;
	}
}
