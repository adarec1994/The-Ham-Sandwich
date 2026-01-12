using MathUtils;
using ProjectWS.Editor;
using ProjectWS.Engine.Data;
using System;
using System.IO;
using System.Windows.Shapes;

namespace ProjectWS.TestArea
{
    public class Tests
    {
        Engine.Engine engine;
        Editor.Editor editor;

        public Tests(Engine.Engine engine, Editor.Editor editor)
        {
            this.engine = engine;
            this.editor = editor;
        }

        /// <summary>
        /// Called when project is launched
        /// </summary>
        public void Start()
        {
            Debug.Log("Starting Tests");

            // Load game data
            // This is normally loaded from the UI
            // but for debugging purposes I need it to load directly at runtime in editor so I don't waste time
            //string installLocation = @"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042\";
            //string cacheLocation = @"D:\Wildstar1.7.8.16042_Cache\";
            //this.engine.LoadGameData(installLocation, OnDataLoaded);
            //this.engine.SetCacheLocation(cacheLocation);

            //CalculateSkyCoeffs(@"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042\Data\Sky\Adventure_Galeras3.sky");

            LoadAnM3ForDebug();
        }

        void OnDataLoaded(Engine.Data.GameData data)
        {
            // Populate world sky list in sky editor pane
            /*
            int idx = 0;
            int defaultIdx = 0;
            foreach (var item in data.database.worldSky.records)
            {
                Program.editor.skyEditorPane.skies.Add(item.Value);
                if (item.Value.ID == 21)
                    defaultIdx = idx;

                idx++;
            }
            Program.editor.skyEditorPane.skyDataGrid.SelectedIndex = defaultIdx;
            */

            //Program.editor.mainForm.m_gameDataWindow.PopulateTreeView(data);
            StartAfterDataLoaded();
        }

        /// <summary>
        /// Called right after the game data and database have finished loading
        /// </summary>
        void StartAfterDataLoaded()
        {
            Debug.Log("Data Loaded");

            //CreateNewWorld();
            //LoadTestWorld();
            //LoadTestTexture();
            //PrintTestDatabase();
            //LoadAnM3ForDebug();
            //TestSkyFile();
        }
        /*
        void LoadTestWorld()
        {
            var testWorld = new Engine.World.World(this.engine, 0, true);
            //testWorld.LoadMap(6, new Vector2(64, 64));   // Map\Eastern, Middle of the map
            testWorld.TeleportToWorldLocation(114, -1); // !tele 3832.459 -1001.444 -4496.945 51
            //testWorld.Teleport(256, -900, 256, 3538, 1); // !tele 256 -998 256 3538
        }
        */
        /*
        void CreateNewWorld()
        {
            this.engine = new Engine.World.World(this.engine, 0, true);
            newWorld.CreateNew("ZeeTest");
        }
        */

        void LoadAnM3ForDebug()
        {
            var ID = 1;
            var renderer = this.editor.CreateRendererPane(Program.mainWindow, "Model", ID, 1, out _);
            //string path0 = @"Art\Character\Chua\Male\Chua_M.m3";
            //string path0 = @"Art\Creature\AgressorBot\AgressorBot.m3";
            //string path0 = @"Art\Prop\Constructed\Quest\Taxi\TaxiKiosk\PRP_Taxi_Kiosk_000.m3";
            //string path0 = @"Art\Prop\Constructed\Ship\Defiance\PRP_Ship_DefianceTransport_000.m3";
            //string path1 = @"Art\Prop\Natural\Tree\Deciduous_RootyMangrove\PRP_Tree_Deciduous_RootyMangrove_Blue_000.m3";
            //string path0 = @"Art\Creature\Asura\Asura.m3";

            //string path0 = @"Art\Prop\Constructed\WalkWays\SanctuaryCommon\PRP_SideWalk_SanctuaryCommonStraight_Brown_001.m3";
            string path0 = @"Art\Creature\Rowsdower\Rowsdower.m3";

            var modelRenderer = renderer as Engine.Rendering.ModelRenderer;
            var model = new Engine.Objects.M3Model(path0, new Vector3(0, 0, 0), this.engine, renderer);
            modelRenderer.AddModel(model);
            (this.editor.modelRendererPanes[ID] as ModelRendererPane).UpdateModelInformation(model);
        }

        void PrintTestDatabase()
        {
            this.engine.data.database.worldLayer.Print();
        }

        void TestSkyFile()
        {
            var path = @"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042\Data\Sky\Cinematics_Arcterra_EXT_TowerMain.sky";
            FileFormats.Sky.File sky = new FileFormats.Sky.File(path);
            using (var str = File.OpenRead(path))
                sky.Read(str);
        }


        float[][] coeffs = new float[9][];

        void CalculateSkyCoeffs(string path)
        {
            for (int i = 0; i < 9; i++)
            {
                coeffs[i] = new float[3];
            }

            FileFormats.Sky.File sky = new FileFormats.Sky.File(path);
            using(var str = File.OpenRead(path))
                sky.Read(str);

            for (int i = 0; i < sky.skyDataBlock0.colorAndAngleUnk0.Length; i++)
            {
                var aandc = sky.skyDataBlock0.colorAndAngleUnk0[i];
                var data = aandc.data[0];
                float mult = 1.0f;
                float[] hdr = new float[] { data.color.X * mult, data.color.Y * mult, data.color.Z * mult };
                //Console.WriteLine("Angle " + data.angle.X + " " + data.angle.Y + " " + data.angle.Z);
                Console.WriteLine("Color " + data.color.X + " " + data.color.Y + " " + data.color.Z);
                float yaw = data.angle.X;
                float pitch = data.angle.Y;

                //float x = MathF.Cos(yaw) * MathF.Cos(pitch);
                //float y = MathF.Sin(yaw) * MathF.Cos(pitch);
                //float z = MathF.Sin(pitch);

                float phi = (float)(yaw * Math.PI / 180d);
                float theta = (float)(pitch * Math.PI / 180d);

                float x = MathF.Sin(theta) * MathF.Cos(phi);         /* Cartesian components */
                float y = MathF.Sin(theta) * MathF.Sin(phi);
                float z = MathF.Cos(theta);

                //float x = MathF.Sin(yaw);
                //float y = -(MathF.Sin(pitch) * MathF.Cos(yaw));
                //float z = -(MathF.Cos(pitch) * MathF.Cos(yaw));

                float domega = 5.65389911f * MathF.Sin(theta);
                updatecoeffs(hdr, domega, x, y, z);
            }

            for (int i = 0; i < 9; i++)
            {
                //Console.WriteLine($"vec3({coeffs[i][0]}, {coeffs[i][1]}, {coeffs[i][2]}),");
            }
            /*
            Console.WriteLine($"cb1_0 = float4({coeffs[0][0]}, {coeffs[0][1]}, {coeffs[0][2]}, {coeffs[1][0]});");
            Console.WriteLine($"cb1_1 = float4({coeffs[1][1]}, {coeffs[1][2]}, {coeffs[2][0]}, {coeffs[2][1]});");
            Console.WriteLine($"cb1_2 = float4({coeffs[2][2]}, {coeffs[3][0]}, {coeffs[3][1]}, {coeffs[3][2]});");

            Console.WriteLine($"cb1_3 = float4({coeffs[4][0]}, {coeffs[4][1]}, {coeffs[4][2]}, {coeffs[5][0]});");
            Console.WriteLine($"cb1_4 = float4({coeffs[5][1]}, {coeffs[5][2]}, {coeffs[6][0]}, {coeffs[6][1]});");
            Console.WriteLine($"cb1_5 = float4({coeffs[6][2]}, {coeffs[7][0]}, {coeffs[7][1]}, {coeffs[7][2]});");

            Console.WriteLine($"cb1_6 = float4({coeffs[8][0]}, {coeffs[8][1]}, {coeffs[8][2]}, 0.00);");
            */


        }


        void updatecoeffs(float[] hdr, float domega, float x, float y, float z)
        {
            /****************************************************************** 
             Update the coefficients (i.e. compute the next term in the
             integral) based on the lighting value hdr[3], the differential
             solid angle domega and cartesian components of surface normal x,y,z

             Inputs:  hdr = L(x,y,z) [note that x^2+y^2+z^2 = 1]
                      i.e. the illumination at position (x,y,z)

                      domega = The solid angle at the pixel corresponding to 
                  (x,y,z).  For these light probes, this is given by 

                  x,y,z  = Cartesian components of surface normal

             Notes:   Of course, there are better numerical methods to do
                      integration, but this naive approach is sufficient for our
                  purpose.

            *********************************************************************/

            int col;
            for (col = 0; col < 3; col++)
            {
                float c; /* A different constant for each coefficient */

                /* L_{00}.  Note that Y_{00} = 0.282095 */
                c = 0.282095f;
                coeffs[0][col] += hdr[col] * c * domega;

                /* L_{1m}. -1 <= m <= 1.  The linear terms */
                c = 0.488603f;
                coeffs[1][col] += hdr[col] * (c * y) * domega;   /* Y_{1-1} = 0.488603 y  */
                coeffs[2][col] += hdr[col] * (c * z) * domega;   /* Y_{10}  = 0.488603 z  */
                coeffs[3][col] += hdr[col] * (c * x) * domega;   /* Y_{11}  = 0.488603 x  */

                /* The Quadratic terms, L_{2m} -2 <= m <= 2 */

                /* First, L_{2-2}, L_{2-1}, L_{21} corresponding to xy,yz,xz */
                c = 1.092548f;
                coeffs[4][col] += hdr[col] * (c * x * y) * domega; /* Y_{2-2} = 1.092548 xy */
                coeffs[5][col] += hdr[col] * (c * y * z) * domega; /* Y_{2-1} = 1.092548 yz */
                coeffs[7][col] += hdr[col] * (c * x * z) * domega; /* Y_{21}  = 1.092548 xz */

                /* L_{20}.  Note that Y_{20} = 0.315392 (3z^2 - 1) */
                c = 0.315392f;
                coeffs[6][col] += hdr[col] * (c * (3 * z * z - 1)) * domega;

                /* L_{22}.  Note that Y_{22} = 0.546274 (x^2 - y^2) */
                c = 0.546274f;
                coeffs[8][col] += hdr[col] * (c * (x * x - y * y)) * domega;
            }
        }

    }
}