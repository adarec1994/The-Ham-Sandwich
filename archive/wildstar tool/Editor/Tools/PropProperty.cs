using MathUtils;
using ProjectWS.Engine.World;
using ProjectWS.FileFormats.Area;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using Xceed.Wpf.Toolkit.PropertyGrid.Attributes;

namespace ProjectWS.Editor.Tools
{
    public partial class PropTool
    {
        public class PropProperty
        {
            public uint uniqueID { get; }
            public uint someID { get; set; }
            public int unk0 { get; set; }
            public int unk1 { get; set; }
            public AreaProp.ModelType modelType { get; }
            //[Category("1.Transform"), Editor(typeof(Vector3Editor), typeof(Vector3Editor))]
            //public List<float> position { get; set; }
            [Category("1.Transform")]
            [DisplayName("Position X")]
            public float positionX { get; set; }
            [Category("1.Transform")]
            [DisplayName("Position Y")]
            public float positionY { get; set; }
            [Category("1.Transform")]
            [DisplayName("Position Z")]
            public float positionZ { get; set; }
            [Category("1.Transform")]
            [DisplayName("Rotation X")]
            public float rotationX { get; set; }
            [Category("1.Transform")]
            [DisplayName("Rotation Y")]
            public float rotationY { get; set; }
            [Category("1.Transform")]
            [DisplayName("Rotation Z")]
            public float rotationZ { get; set; }
            [Category("1.Transform")]
            [DisplayName("Scale")]
            public float scale { get; set; }
            [ExpandableObject] public PlacementProperty placement { get; }
            public int unk7 { get; set; }
            public int unk8 { get; set; }
            public int unk9 { get; set; }
            [Category("2.Color")] public System.Windows.Media.Color color0 { get; set; }
            [Category("2.Color")] public System.Windows.Media.Color color1 { get; set; }
            public int unk10 { get; set; }
            public int unk11 { get; set; }
            [Category("2.Color")] public System.Windows.Media.Color color2 { get; set; }
            public int unk12 { get; set; }
            public string? path { get; }

            public void Refresh(ref Prop.Instance instance)
            {
                if (instance.areaprop != null)
                {
                    instance.areaprop.position = new Vector3(this.positionX, this.positionY, this.positionZ);
                    instance.areaprop.rotation = Quaternion.FromEulerAngles((float)(Math.PI / 180) * this.rotationX, (float)(Math.PI / 180) * this.rotationY, (float)(Math.PI / 180) * this.rotationZ);
                    instance.areaprop.scale = this.scale;

                    instance.areaprop.color0 = new Color32(this.color0.R, this.color0.G, this.color0.B, this.color0.A);
                    instance.areaprop.color1 = new Color32(this.color1.R, this.color1.G, this.color1.B, this.color1.A);
                    instance.areaprop.color2 = new Color32(this.color2.R, this.color2.G, this.color2.B, this.color2.A);

                    instance.position = instance.areaprop.position;
                    instance.rotation = instance.areaprop.rotation;
                    instance.scale = this.scale * Vector3.One;

                    Matrix4 mat = Matrix4.Identity;
                    instance.transform = mat.TRS(instance.areaprop.position, instance.areaprop.rotation, this.scale * Vector3.One);
                }
                Console.WriteLine("REFRESH");
            }

            public PropProperty(AreaProp areaprop)
            {
                this.uniqueID = areaprop.uniqueID;
                this.someID = areaprop.someID;
                this.unk0 = areaprop.unk0;
                this.unk1 = areaprop.unk1;
                this.modelType = areaprop.modelType;
                this.scale = areaprop.scale;
                //this.rotation = areaprop.rotation;
                var euler = areaprop.rotation.ToEulerAngles() * (float)(180 / Math.PI);
                this.rotationX = euler.X;
                this.rotationY = euler.Y;
                this.rotationZ = euler.Z;
                //this.position = new List<float> { areaprop.position.X, areaprop.position.Y, areaprop.position.Z };
                this.positionX = areaprop.position.X;
                this.positionY = areaprop.position.Y;
                this.positionZ = areaprop.position.Z;
                this.placement =  new PlacementProperty(areaprop.placement);
                this.unk7 = areaprop.unk7;
                this.unk8 = areaprop.unk8;
                this.unk9 = areaprop.unk9;
                this.color0 = new System.Windows.Media.Color { R = areaprop.color0.R, G = areaprop.color0.G, B = areaprop.color0.B, A = areaprop.color0.A };
                this.color1 = new System.Windows.Media.Color { R = areaprop.color1.R, G = areaprop.color1.G, B = areaprop.color1.B, A = areaprop.color1.A };
                this.unk10 = areaprop.unk10;
                this.unk11 = areaprop.unk11;
                this.color2 = new System.Windows.Media.Color { R = areaprop.color2.R, G = areaprop.color2.G, B = areaprop.color2.B, A = areaprop.color2.A };
                this.unk12 = areaprop.unk12;
                this.path = areaprop.path;
            }
        }
    }
}
