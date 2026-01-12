namespace ProjectWS.Engine.Database.Definitions
{
	public class Class : TblRecord
	{
		public override string GetFileName() => "Class";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string enumName;
		public uint localizedTextId;
		public uint localizedTextIdNameFemale;
		public uint mechanic;
		public uint spell4IdInnateAbilityActive00;
		public uint spell4IdInnateAbilityActive01;
		public uint spell4IdInnateAbilityActive02;
		public uint spell4IdInnateAbilityPassive00;
		public uint spell4IdInnateAbilityPassive01;
		public uint spell4IdInnateAbilityPassive02;
		public uint prerequisiteIdInnateAbility00;
		public uint prerequisiteIdInnateAbility01;
		public uint prerequisiteIdInnateAbility02;
		public uint startingItemProficiencies;
		public uint spell4IdAttackPrimary00;
		public uint spell4IdAttackPrimary01;
		public uint spell4IdAttackUnarmed00;
		public uint spell4IdAttackUnarmed01;
		public uint spell4IdResAbility;
		public uint localizedTextIdDescription;
		public uint spell4GroupId;
		public uint classIdForClassApModifier;
		public uint vitalEnumResource00;
		public uint vitalEnumResource01;
		public uint vitalEnumResource02;
		public uint vitalEnumResource03;
		public uint vitalEnumResource04;
		public uint vitalEnumResource05;
		public uint vitalEnumResource06;
		public uint vitalEnumResource07;
		public uint localizedTextIdResourceAlert00;
		public uint localizedTextIdResourceAlert01;
		public uint localizedTextIdResourceAlert02;
		public uint localizedTextIdResourceAlert03;
		public uint localizedTextIdResourceAlert04;
		public uint localizedTextIdResourceAlert05;
		public uint localizedTextIdResourceAlert06;
		public uint localizedTextIdResourceAlert07;
		public uint attributeMilestoneGroupId00;
		public uint attributeMilestoneGroupId01;
		public uint attributeMilestoneGroupId02;
		public uint attributeMilestoneGroupId03;
		public uint attributeMilestoneGroupId04;
		public uint attributeMilestoneGroupId05;
		public uint classSecondaryStatBonusId00;
		public uint classSecondaryStatBonusId01;
		public uint classSecondaryStatBonusId02;
		public uint classSecondaryStatBonusId03;
		public uint classSecondaryStatBonusId04;
		public uint classSecondaryStatBonusId05;
		public uint attributeMiniMilestoneGroupId00;
		public uint attributeMiniMilestoneGroupId01;
		public uint attributeMiniMilestoneGroupId02;
		public uint attributeMiniMilestoneGroupId03;
		public uint attributeMiniMilestoneGroupId04;
		public uint attributeMiniMilestoneGroupId05;
		public uint attributeMilestoneMaxTiers00;
		public uint attributeMilestoneMaxTiers01;
		public uint attributeMilestoneMaxTiers02;
		public uint attributeMilestoneMaxTiers03;
		public uint attributeMilestoneMaxTiers04;
		public uint attributeMilestoneMaxTiers05;
	}
}
