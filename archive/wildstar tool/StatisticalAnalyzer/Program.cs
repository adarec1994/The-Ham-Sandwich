using System.Diagnostics;
using System.Numerics;

namespace StatisticalAnalyzer
{
    internal class Program
    {

        static void Main(string[] args)
        {
            Console.ForegroundColor = ConsoleColor.Gray;
            //CheckM3Files(@"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042 Extracted\Art\");
            //CheckAreaFiles(@"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042 Extracted\Map\Eastern");
            //ExtractShaders(@"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042 Extracted\Shaders", @"G:\Reverse Engineering\WildStar\DecompiledShaders");
            //CheckSkyFiles(@"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042 Extracted\Sky");
            //CheckTexFiles(@"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042 Extracted\Art\");
            //var area = new ProjectWS.Engine.Data.Area(@"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042 Extracted\Map\BattlegroundHallsoftheBloodsworn\BattlegroundHallsoftheBloodsworn.3f3d.area");
            //area.Read();
            CheckI3Files(@"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042 Extracted\Art");
            //CalculateSkyCoeffs(@"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042\Data\Sky\Adventure_Galeras3.sky");
        }

        private static void CheckI3Files(string location)
        {
            string[] i3Files = Directory.GetFiles(location, "*.i3", SearchOption.AllDirectories);

            HashSet<long> collected = new HashSet<long>();
            //Console.WriteLine($"{i3Files.Length} files.");
            foreach (var filePath in i3Files)
            {
                var i3 = new ProjectWS.FileFormats.I3.I3File(filePath);
                using (var str = File.OpenRead(filePath))
                {
                    i3.Read(str);
                }

                if (i3.failedReading)
                    continue;
                /*
                var val = i3.unk2;

                if (!collected.Contains(val))
                {
                    collected.Add(val);
                    //Console.WriteLine(filePath);
                }
                */
                if (i3.unk1?.Length != 0)
                    Console.WriteLine(filePath);
                /*
                for (int g = 0; g < i3.unk5?.Length; g++)
                {
                    var val = i3.unk5[g].unk15;

                    if (!collected.Contains(val))
                    {
                        collected.Add(val);
                        //Console.WriteLine(filePath);
                    }
                }
                */
            }

            foreach (var item in collected)
            {
                Console.WriteLine(item);
            }
        }

        static float[][] coeffs = new float[9][];

        static void CalculateSkyCoeffs(string path)
        {
            for (int i = 0; i < 9; i++)
            {
                coeffs[i] = new float[3];
            }

            ProjectWS.FileFormats.Sky.File sky = new ProjectWS.FileFormats.Sky.File(path);
            using(var fs = File.OpenRead(path))
                sky.Read(fs);

            Vector4[] colors = new Vector4[]
            {
                new Vector4(0.0f, 0.0f, 0.0f, 1.0f),
                new Vector4(1.0f, 0.0f, 0.0f, 1.0f),
                new Vector4(0.0f, 0.0f, 0.0f, 1.0f),
                new Vector4(0.0f, 0.0f, 0.0f, 1.0f),
                new Vector4(0.0f, 0.0f, 0.0f, 1.0f),
                new Vector4(0.0f, 0.0f, 0.0f, 1.0f),
            };

            Vector4[] angles = new Vector4[]
            {
                new Vector4(0.0f, 0.0f, 0.0f, 0.0f),
                new Vector4(0.0f, 0.0f, 0.0f, 0.0f),
                new Vector4(0, 0, 0.0f, 0.0f),
                new Vector4(0, 0, 0.0f, 0.0f),
                new Vector4(0, 0, 0.0f, 0.0f),
                new Vector4(0, 0, 0.0f, 0.0f),
            };

            //for (int i = 0; i < sky.skyDataBlock0.angleAndColorUnk0.Length; i++)
            for (int i = 0; i < 6; i++)
            {
                Vector4 angle = angles[i];
                float[] hdr = new float[] { colors[i].X, colors[i].Y, colors[i].Z };
                float yaw = angle.X;
                float pitch = angle.Y;
                /*
                var aandc = sky.skyDataBlock0.angleAndColorUnk0[i];
                var data = aandc.data[0];
                float mult = 1.0f;
                float[] hdr = new float[] { data.color.X * mult, data.color.Y * mult, data.color.Z * mult };
                //Console.WriteLine("Angle " + data.angle.X + " " + data.angle.Y + " " + data.angle.Z);
                Console.WriteLine("Color " + data.color.X + " " + data.color.Y + " " + data.color.Z);
                float yaw = data.angle.X;
                float pitch = data.angle.Y;
                */

                //float x = MathF.Cos(yaw) * MathF.Cos(pitch);
                //float y = MathF.Sin(yaw) * MathF.Cos(pitch);
                //float z = MathF.Sin(pitch);

                float phi = yaw * MathF.PI / 180f;
                float theta = pitch * MathF.PI / 180f;

                float x = MathF.Sin(theta) * MathF.Cos(phi);         /* Cartesian components */
                float y = MathF.Sin(theta) * MathF.Sin(phi);
                float z = MathF.Cos(theta);

                //float x = MathF.Sin(yaw);
                //float y = -(MathF.Sin(pitch) * MathF.Cos(yaw));
                //float z = -(MathF.Cos(pitch) * MathF.Cos(yaw));

                float domega = 1.0f;// MathF.Sin(theta);
                updatecoeffs(hdr, domega, x, y, z);
            }

            for (int i = 0; i < 9; i++)
            {
                //Console.WriteLine($"vec3({coeffs[i][0]}, {coeffs[i][1]}, {coeffs[i][2]}),");
            }

            Console.WriteLine($"cb1_0 = float4({coeffs[0][0]}, {coeffs[0][1]}, {coeffs[0][2]}, {coeffs[1][0]});");
            Console.WriteLine($"cb1_1 = float4({coeffs[1][1]}, {coeffs[1][2]}, {coeffs[2][0]}, {coeffs[2][1]});");
            Console.WriteLine($"cb1_2 = float4({coeffs[2][2]}, {coeffs[3][0]}, {coeffs[3][1]}, {coeffs[3][2]});");

            Console.WriteLine($"cb1_3 = float4({coeffs[4][0]}, {coeffs[4][1]}, {coeffs[4][2]}, {coeffs[5][0]});");
            Console.WriteLine($"cb1_4 = float4({coeffs[5][1]}, {coeffs[5][2]}, {coeffs[6][0]}, {coeffs[6][1]});");
            Console.WriteLine($"cb1_5 = float4({coeffs[6][2]}, {coeffs[7][0]}, {coeffs[7][1]}, {coeffs[7][2]});");

            Console.WriteLine($"cb1_6 = float4({coeffs[8][0]}, {coeffs[8][1]}, {coeffs[8][2]}, 0.00);");



        }


        static void updatecoeffs(float[] hdr, float domega, float x, float y, float z)
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
                //c = 0.488603f;
                c = 0.30765f;
                coeffs[1][col] += hdr[col] * (c * y) * domega;   /* Y_{1-1} = 0.488603 y  */
                coeffs[2][col] += hdr[col] * (c * z) * domega;   /* Y_{10}  = 0.488603 z  */
                coeffs[3][col] += hdr[col] * (c * x) * domega;   /* Y_{11}  = 0.488603 x  */

                /* The Quadratic terms, L_{2m} -2 <= m <= 2 */

                /* First, L_{2-2}, L_{2-1}, L_{21} corresponding to xy,yz,xz */
                //c = 1.092548f;
                coeffs[4][col] += hdr[col] * (c * x * y) * domega; /* Y_{2-2} = 1.092548 xy */
                coeffs[5][col] += hdr[col] * (c * y * z) * domega; /* Y_{2-1} = 1.092548 yz */
                coeffs[7][col] += hdr[col] * (c * x * z) * domega; /* Y_{21}  = 1.092548 xz */

                /* L_{20}.  Note that Y_{20} = 0.315392 (3z^2 - 1) */
                //c = 0.315392f;
                coeffs[6][col] += hdr[col] * (c * (3 * z * z - 1)) * domega;

                /* L_{22}.  Note that Y_{22} = 0.546274 (x^2 - y^2) */
                //c = 0.546274f;
                coeffs[8][col] += hdr[col] * (c * (x * x - y * y)) * domega;
            }
        }


        static void CheckM3Files(string location)
        {
            string[] m3Files = Directory.GetFiles(location, "*.m3", SearchOption.AllDirectories);

            HashSet<long> collected = new HashSet<long>();

            foreach (var filePath in m3Files)
            {
                var m3 = new ProjectWS.FileFormats.M3.File(filePath);
                using (var str = File.OpenRead(filePath))
                {
                    m3.Read(str);
                }

                if (m3.failedReading)
                    continue;

                for (int g = 0; g < m3.bounds.Length; g++)
                {
                    /*
                    var val = m3.bounds[g].bbA;

                    if (!collected.Contains(val))
                    {
                        collected.Add(val);
                        //Console.WriteLine(filePath);
                    }
                    */
                }

                //if (!collected.Contains(m3.unk1a))
                //    collected.Add(m3.unk1a);

                /*
                if (m3.unk100 != null && m3.unk100.data.Length > 0)
                {
                    Console.WriteLine(filePath);
                }
                */
            }

            foreach (var item in collected)
            {
                Console.WriteLine(item);
            }
        }

        static void CheckTexFiles(string location)
        {
            string[] texFiles = Directory.GetFiles(location, "*.tex", SearchOption.AllDirectories);

            HashSet<long> collected = new HashSet<long>();

            foreach (var filePath in texFiles)
            {
                var tex = new ProjectWS.FileFormats.Tex.File(filePath);
                using (var str = File.OpenRead(filePath))
                {
                    if (str.Length == 0) continue;
                    //tex.Read(str);
                    using (BinaryReader br = new BinaryReader(str))
                    {
                        var header = new ProjectWS.FileFormats.Tex.Header(br);
                        if (header.unk != 0)
                            Console.WriteLine(header.unk + " " + filePath);
                    }
                }
            }
        }

        static void CheckAreaFiles(string location)
        {
            string[] areaFiles = Directory.GetFiles(location, "*.area", SearchOption.AllDirectories);

            HashSet<long> collected = new HashSet<long>();

            ushort min = ushort.MaxValue;
            ushort max = ushort.MinValue;

            foreach (var filePath in areaFiles)
            {
                if (filePath.Contains("_Low")) continue;

                var area = new ProjectWS.FileFormats.Area.File(filePath);
                using(var str = File.OpenRead(filePath))
                    area.Read(str);

                for (int i = 0; i < area.subAreas?.Count; i++)
                {
                    for (int h = 0; h < area.subAreas[i].heightMap?.Length; h++)
                    {
                        ushort height = area.subAreas[i].heightMap[h];

                        if (height < min)
                            min = height;

                        if (height > max)
                            max = height;
                    }
                    /*
                    var check = area.subAreas[i].index;
                    //Console.WriteLine(filePath);

                    if (!collected.Contains(check))
                    {
                        collected.Add(check);
                    }
                    */
                }
            }

            float hmin = ((min & 0x7FFF) * 0.12500381f) - 2048.0f;
            float hmax = ((max & 0x7FFF) * 0.12500381f) - 2048.0f;
            Console.WriteLine(min + " " + max);
            Console.WriteLine(hmin + " " + hmax);

            foreach (var item in collected)
            {
                Console.WriteLine(item);
            }
        }

        static void CheckSkyFiles(string location)
        {
            // G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042 Extracted\AIDX\Sky\Arcterra_EXT_Bones.sky
            string[] areaFiles = Directory.GetFiles(location, "*.sky", SearchOption.AllDirectories);

            HashSet<float> collected = new HashSet<float>();

            foreach (var filePath in areaFiles)
            {
                var sky = new ProjectWS.FileFormats.Sky.File(filePath);
                using(var fs = File.OpenRead(filePath))
                    sky.Read(fs);

                if (sky.fogSettings.timestamps.Length > 0)
                {
                    Console.WriteLine(filePath);
                }

                /*
                if (sky.sunLightColor.timestamps.Length > 0)
                {
                    var check = sky.sunLightColor.timestamps[0];

                    if (!collected.Contains(check))
                    {
                        collected.Add(check);
                    }
                }
                */
            }

            foreach (var item in collected)
            {
                Console.WriteLine(item);
            }
        }

        static void ExtractShaders(string shoPath, string outputPath)
        {
            string[] metafiles = Directory.GetFiles(shoPath, "*_0*");

            for (int i = 0; i < metafiles.Length; i++)
            {
                var fileName = Path.GetFileNameWithoutExtension(metafiles[i]);
                Console.WriteLine(fileName);

                try
                {
                    ProjectWS.FileFormats.Sho.File sho = new ProjectWS.FileFormats.Sho.File(metafiles[i]);
                    sho.Read();
                    var folder = $"{outputPath}/{fileName}";

                    if (!Directory.Exists(folder))
                        Directory.CreateDirectory(folder);

                    if (sho.variants != null)
                    {
                        for (int v = 0; v < sho.variants.Length; v++)
                        {
                            if (sho.variants[v] != null)
                            {
                                var dxbcPath = $"{folder}/{fileName}_{v:D3}.dxbc";
                                var glslPath = $"{folder}/{fileName}_{v:D3}.glsl";
                                if (sho.variants.Length == 1)
                                {
                                    dxbcPath = $"{folder}/{fileName}.dxbc";
                                    glslPath = $"{folder}/{fileName}.glsl";
                                }

                                if (sho.variants[v].data != null)
                                {
                                    File.WriteAllBytes(dxbcPath, sho.variants[v].data);
                                    DXBC2HLSL(dxbcPath, glslPath);
                                    //File.Delete(dxbcPath);
                                }
                            }
                        }
                    }

                    if (IsDirectoryEmpty(folder))
                        Directory.Delete(folder);
                }
                catch (Exception ex)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine(ex.ToString());
                    Console.WriteLine(metafiles[i]);
                    Console.ResetColor();
                    return;
                }
            }
        }

        static void DXBC2HLSL(string inputFile, string outputFile)
        {
            string execPath = AppDomain.CurrentDomain.BaseDirectory;
            string dxilSpirv = execPath + @"/DXBCDecompile/dxil-spirv.exe";

            var startInfo = new ProcessStartInfo
            {
                FileName = dxilSpirv,
                Arguments = $"\"{inputFile}\" --glsl --output \"{outputFile}\"",
                //UseShellExecute = false,
                RedirectStandardOutput = true,
                CreateNoWindow = true
            };

            using var process = Process.Start(startInfo);

            Stopwatch timer = new Stopwatch();
            timer.Start();

            while (true)
            {
                if (process != null)
                {
                    if (!process.StandardOutput.EndOfStream)
                    {
                        //string line = process.StandardOutput.ReadLine();
                    }

                    if (timer.Elapsed.TotalSeconds >= 60) // 1 minute
                    {
                        process.CloseMainWindow();
                        process.Close();
                        break;
                    }

                    if (process.HasExited)
                        break;
                }
            }

            timer.Stop();
        }

        static bool IsDirectoryEmpty(string path)
        {
            IEnumerable<string> items = Directory.EnumerateFileSystemEntries(path);
            using (IEnumerator<string> en = items.GetEnumerator())
            {
                return !en.MoveNext();
            }
        }

    }
}