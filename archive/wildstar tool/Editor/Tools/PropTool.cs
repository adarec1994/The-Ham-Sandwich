using MathUtils;
using ProjectWS.Editor.UI.Toolbox;
using ProjectWS.Engine.Rendering;
using ProjectWS.Engine.World;

namespace ProjectWS.Editor.Tools
{
    public partial class PropTool : Tool
    {
        readonly Engine.Engine? engine;
        public readonly WorldRenderer? worldRenderer;
        readonly Editor? editor;
        Prop? currentProp;
        Prop.Instance? currentPropInstance;
        PropProperty? selectedPropProperty;

        public PropTool(Engine.Engine engine, Editor editor, WorldRenderer world)
        {
            this.hasBrush = true;
            this.editor = editor;
            this.engine = engine;
            this.worldRenderer = world;
            this.isEnabled = false;
        }

        public override void OnTooboxPaneLoaded()
        {
            if (this.editor?.toolboxPane != null)
            {
                TerrainPropPlacePane toolPane = this.editor.toolboxPane.terrainPropPlacePane;

                toolPane.propertyGrid_prop.PropertyValueChanged += (obj, args) =>
                {
                    this.selectedPropProperty.Refresh(ref this.currentPropInstance);
                };
            }
        }

        public override void Enable()
        {
            this.isEnabled = true;
            if (this.worldRenderer != null && this.worldRenderer.mousePick != null)
                this.worldRenderer.mousePick.mode = Engine.MousePick.Mode.Prop;
        }

        public override void Disable()
        {
            this.isEnabled = false;
            if (this.worldRenderer != null && this.worldRenderer.mousePick != null)
                this.worldRenderer.mousePick.mode = Engine.MousePick.Mode.Disabled;
        }

        public override void Update(float deltaTime)
        {
            if (this.worldRenderer == null) return;
            if (this.worldRenderer.world == null) return;
            if (this.worldRenderer.mousePick == null) return;

            if (this.isEnabled && this.editor != null && this.editor.keyboardFocused)
            {
                var propHit = this.worldRenderer.mousePick.propHit;
                var propInstanceHit = this.worldRenderer.mousePick.propInstanceHit;

                if (propHit != this.currentProp || propInstanceHit != this.currentPropInstance)
                {
                    this.currentProp = propHit;
                    this.currentPropInstance = propInstanceHit;
                    OnPropSelectionChanged(this.worldRenderer.world, propHit, propInstanceHit);
                }

                if (propHit != null && propInstanceHit != null)
                    if (propInstanceHit.obb != null && propInstanceHit.areaprop != null)
                        DrawOBB(propInstanceHit.obb, propInstanceHit.transform, Color32.Yellow);
            }
        }

        internal void DrawOBB(OBB obb, Matrix4 transform, Color32 color)
        {
            var positionOffsetMat = Matrix4.CreateTranslation(obb.center);
            var scaleMat = Matrix4.CreateScale(obb.size);
            var boxMat = scaleMat * positionOffsetMat * transform;

            Debug.DrawWireBox3D(boxMat, color);
        }

        public void OnPropSelectionChanged(World world, Prop? prop, Prop.Instance? propInstance)
        {
            if (this.editor == null) return;
            if (this.editor.toolboxPane == null) return;

            TerrainPropPlacePane toolPane = this.editor.toolboxPane.terrainPropPlacePane;

            if (prop != null && propInstance != null)
            {
                if (prop.data != null)
                {
                    toolPane.textBlock_SelectedProp.Text = prop.data.fileName;
                }

                if (propInstance.areaprop != null)
                {
                    this.selectedPropProperty = new PropProperty(propInstance.areaprop);

                    toolPane.propertyGrid_prop.SelectedObject = this.selectedPropProperty;
                }
            }
            else
            {
                toolPane.textBlock_SelectedProp.Text = string.Empty;
                this.selectedPropProperty = null;
                toolPane.propertyGrid_prop.SelectedObject = null;
            }
        }
    }
}
