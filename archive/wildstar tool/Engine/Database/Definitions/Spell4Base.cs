namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Base : TblRecord
	{
		public override string GetFileName() => "Spell4Base";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint spell4HitResultId;
		public uint spell4TargetMechanicId;
		public uint spell4TargetAngleId;
		public uint spell4PrerequisiteId;
		public uint spell4ValidTargetId;
		public uint targetGroupIdCastGroup;
		public uint creature2IdPositionalAoe;
		public float parameterAEAngle;
		public float parameterAEMaxAngle;
		public float parameterAEDistance;
		public float parameterAEMaxDistance;
		public uint targetGroupIdAoeGroup;
		public uint spell4BaseIdPrerequisiteSpell;
		public uint worldZoneIdZoneRequired;
		public uint spell4SpellTypesIdSpellType;
		public string icon;
		public uint castMethod;
		public uint school;
		public uint spellClass;
		public uint weaponSlot;
		public uint castBarType;
		public float mechanicAggressionMagnitude;
		public float mechanicDominationMagnitude;
		public uint modelSequencePriorityCaster;
		public uint modelSequencePriorityTarget;
		public uint classIdPlayer;
		public uint clientSideInteractionId;
		public uint targetingFlags;
		public uint telegraphFlagsEnum;
		public uint localizedTextIdLASTierPoint;
		public float lasTierPointTooltipData00;
		public float lasTierPointTooltipData01;
		public float lasTierPointTooltipData02;
		public float lasTierPointTooltipData03;
		public float lasTierPointTooltipData04;
		public float lasTierPointTooltipData05;
		public float lasTierPointTooltipData06;
		public float lasTierPointTooltipData07;
		public float lasTierPointTooltipData08;
		public float lasTierPointTooltipData09;
	}
}
