namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemDisplay : TblRecord
	{
		public override string GetFileName() => "ItemDisplay";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public uint item2TypeId;
		public string objectModel;
		public string objectModelL;
		public string objectTexture0;
		public string objectTexture1;
		public uint modelTextureIdObject00;
		public uint modelTextureIdObject01;
		public string skinnedModel;
		public string skinnedModelL;
		public string skinnedTexture0;
		public string skinnedTexture1;
		public uint modelTextureIdSkinned00;
		public uint modelTextureIdSkinned01;
		public string attachedModel;
		public uint modelAttachmentIdAttached;
		public string attachedTexture0;
		public string attachedTexture1;
		public uint modelTextureIdAttached00;
		public uint modelTextureIdAttached01;
		public uint componentRegionFlags;
		public uint componentPriority;
		public string skinMaskMap;
		public string skinColorMap;
		public string skinNormalMap;
		public string skinDyeMap;
		public string armorMaskMap;
		public string armorColorMap;
		public string armorNormalMap;
		public string armorDyeMap;
		public uint modelMeshId00;
		public uint modelMeshId01;
		public uint modelMeshId02;
		public uint modelMeshId03;
		public uint soundImpactDescriptionId;
		public uint ItemVisualTypeId;
		public uint soundReplaceDescriptionId;
		public uint itemColorSetId;
		public uint dyeChannelFlags;
		public uint modelPoseId;
		public float modelPoseBlend;
		public uint shaderPreset00;
		public uint shaderPreset01;
	}
}
