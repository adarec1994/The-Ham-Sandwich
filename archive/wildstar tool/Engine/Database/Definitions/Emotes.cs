namespace ProjectWS.Engine.Database.Definitions
{
	public class Emotes : TblRecord
	{
		public override string GetFileName() => "Emotes";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdNoArgToAll;
		public uint localizedTextIdNoArgToSelf;
		public uint NoArgAnim;
		public uint localizedTextIdArgToAll;
		public uint localizedTextIdArgToArg;
		public uint localizedTextIdArgToSelf;
		public uint ArgAnim;
		public uint localizedTextIdSelfToAll;
		public uint localizedTextIdSelfToSelf;
		public uint SelfAnim;
		public uint SheathWeapons;
		public uint TurnToFace;
		public uint TextReplaceable;
		public uint ChangesStandState;
		public uint StandState;
		public uint localizedTextIdCommand;
		public uint localizedTextIdNotFoundToAll;
		public uint localizedTextIdNotFoundToSelf;
		public uint NotFoundAnim;
		public uint TextReplaceAnim;
		public uint modelSequenceIdStandState;
		public uint visualEffectId;
		public uint flags;
		public string universalCommand00;
		public string universalCommand01;
	}
}
