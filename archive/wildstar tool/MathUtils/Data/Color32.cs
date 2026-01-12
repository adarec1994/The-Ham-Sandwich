//
// The Open Toolkit Library License
//
// Copyright (c) 2006 - 2008 the Open Toolkit library, except where noted.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//

using System.Diagnostics.Contracts;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace MathUtils
{
    /// <summary>
    /// Represents a color with 4 floating-point components (R, G, B, A).
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct Color32 : IEquatable<Color32>
    {
        public byte R;
        public byte G;
        public byte B;
        public byte A;

        /// <summary>
        /// Initializes a new instance of the <see cref="Color32"/> struct.
        /// </summary>
        /// <param name="r">The red component of the new Color32 structure.</param>
        /// <param name="g">The green component of the new Color32 structure.</param>
        /// <param name="b">The blue component of the new Color32 structure.</param>
        /// <param name="a">The alpha component of the new Color32 structure.</param>
        public Color32(byte r, byte g, byte b, byte a)
        {
            R = r;
            G = g;
            B = b;
            A = a;
        }

        /// <summary>
        /// Compares the specified Color32 structures for equality.
        /// </summary>
        /// <param name="left">The left-hand side of the comparison.</param>
        /// <param name="right">The right-hand side of the comparison.</param>
        /// <returns>True if left is equal to right; false otherwise.</returns>
        [Pure]
        public static bool operator ==(Color32 left, Color32 right)
        {
            return left.Equals(right);
        }

        /// <summary>
        /// Compares the specified Color32 structures for inequality.
        /// </summary>
        /// <param name="left">The left-hand side of the comparison.</param>
        /// <param name="right">The right-hand side of the comparison.</param>
        /// <returns>True if left is not equal to right; false otherwise.</returns>
        [Pure]
        public static bool operator !=(Color32 left, Color32 right)
        {
            return !left.Equals(right);
        }

        [Pure]
        public static implicit operator Color32(Color color)
        {
            return new Color32((byte)(color.R * 255), (byte)(color.G * 255), (byte)(color.B * 255), (byte)(color.A * 255));
        }

        /// <summary>
        /// Returns this Color32 as a Vector4. The resulting struct will have XYZW mapped to RGBA, in that order.
        /// </summary>
        /// <param name="c">The Color32 to convert.</param>
        /// <returns>The Color32, converted into a Vector4.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        [Pure]
        public static explicit operator Vector4(Color32 c)
        {
            return new Vector4(c.R / 255f, c.G / 255f, c.B / 255f, c.A / 255f);
        }

        public override bool Equals(object obj)
        {
            return obj is Color32 && Equals((Color32)obj);
        }

        /// <summary>
        /// Calculates the hash code for this Color32 structure.
        /// </summary>
        /// <returns>A System.Int32 containing the hashcode of this Color32 structure.</returns>
        public override int GetHashCode()
        {
            return HashCode.Combine(R, G, B, A);
        }

        /// <summary>
        /// Creates a System.String that describes this Color32 structure.
        /// </summary>
        /// <returns>A System.String that describes this Color32 structure.</returns>
        public override string ToString()
        {
            var ls = MathHelper.ListSeparator;
            return $"{{(R{ls} G{ls} B{ls} A) = ({R}{ls} {G}{ls} {B}{ls} {A})}}";
        }

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 255, 255, 0).
        /// </summary>
        public static Color32 Transparent => new Color32(255, 255, 255, 0);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (240, 248, 255, 255).
        /// </summary>
        public static Color32 AliceBlue => new Color32(240, 248, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (250, 235, 215, 255).
        /// </summary>
        public static Color32 AntiqueWhite => new Color32(250, 235, 215, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 255, 255, 255).
        /// </summary>
        public static Color32 Aqua => new Color32(0, 255, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (127, 255, 212, 255).
        /// </summary>
        public static Color32 Aquamarine => new Color32(127, 255, 212, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (240, 255, 255, 255).
        /// </summary>
        public static Color32 Azure => new Color32(240, 255, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (245, 245, 220, 255).
        /// </summary>
        public static Color32 Beige => new Color32(245, 245, 220, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 228, 196, 255).
        /// </summary>
        public static Color32 Bisque => new Color32(255, 228, 196, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 0, 0, 255).
        /// </summary>
        public static Color32 Black => new Color32(0, 0, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 235, 205, 255).
        /// </summary>
        public static Color32 BlanchedAlmond => new Color32(255, 235, 205, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 0, 255, 255).
        /// </summary>
        public static Color32 Blue => new Color32(0, 0, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (138, 43, 226, 255).
        /// </summary>
        public static Color32 BlueViolet => new Color32(138, 43, 226, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (165, 42, 42, 255).
        /// </summary>
        public static Color32 Brown => new Color32(165, 42, 42, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (222, 184, 135, 255).
        /// </summary>
        public static Color32 BurlyWood => new Color32(222, 184, 135, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (95, 158, 160, 255).
        /// </summary>
        public static Color32 CadetBlue => new Color32(95, 158, 160, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (127, 255, 0, 255).
        /// </summary>
        public static Color32 Chartreuse => new Color32(127, 255, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (210, 105, 30, 255).
        /// </summary>
        public static Color32 Chocolate => new Color32(210, 105, 30, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 127, 80, 255).
        /// </summary>
        public static Color32 Coral => new Color32(255, 127, 80, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (100, 149, 237, 255).
        /// </summary>
        public static Color32 CornflowerBlue => new Color32(100, 149, 237, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 248, 220, 255).
        /// </summary>
        public static Color32 Cornsilk => new Color32(255, 248, 220, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (220, 20, 60, 255).
        /// </summary>
        public static Color32 Crimson => new Color32(220, 20, 60, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 255, 255, 255).
        /// </summary>
        public static Color32 Cyan => new Color32(0, 255, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 0, 139, 255).
        /// </summary>
        public static Color32 DarkBlue => new Color32(0, 0, 139, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 139, 139, 255).
        /// </summary>
        public static Color32 DarkCyan => new Color32(0, 139, 139, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (184, 134, 11, 255).
        /// </summary>
        public static Color32 DarkGoldenrod => new Color32(184, 134, 11, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (169, 169, 169, 255).
        /// </summary>
        public static Color32 DarkGray => new Color32(169, 169, 169, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 100, 0, 255).
        /// </summary>
        public static Color32 DarkGreen => new Color32(0, 100, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (189, 183, 107, 255).
        /// </summary>
        public static Color32 DarkKhaki => new Color32(189, 183, 107, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (139, 0, 139, 255).
        /// </summary>
        public static Color32 DarkMagenta => new Color32(139, 0, 139, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (85, 107, 47, 255).
        /// </summary>
        public static Color32 DarkOliveGreen => new Color32(85, 107, 47, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 140, 0, 255).
        /// </summary>
        public static Color32 DarkOrange => new Color32(255, 140, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (153, 50, 204, 255).
        /// </summary>
        public static Color32 DarkOrchid => new Color32(153, 50, 204, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (139, 0, 0, 255).
        /// </summary>
        public static Color32 DarkRed => new Color32(139, 0, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (233, 150, 122, 255).
        /// </summary>
        public static Color32 DarkSalmon => new Color32(233, 150, 122, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (143, 188, 139, 255).
        /// </summary>
        public static Color32 DarkSeaGreen => new Color32(143, 188, 139, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (72, 61, 139, 255).
        /// </summary>
        public static Color32 DarkSlateBlue => new Color32(72, 61, 139, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (47, 79, 79, 255).
        /// </summary>
        public static Color32 DarkSlateGray => new Color32(47, 79, 79, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 206, 209, 255).
        /// </summary>
        public static Color32 DarkTurquoise => new Color32(0, 206, 209, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (148, 0, 211, 255).
        /// </summary>
        public static Color32 DarkViolet => new Color32(148, 0, 211, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 20, 147, 255).
        /// </summary>
        public static Color32 DeepPink => new Color32(255, 20, 147, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 191, 255, 255).
        /// </summary>
        public static Color32 DeepSkyBlue => new Color32(0, 191, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (105, 105, 105, 255).
        /// </summary>
        public static Color32 DimGray => new Color32(105, 105, 105, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (30, 144, 255, 255).
        /// </summary>
        public static Color32 DodgerBlue => new Color32(30, 144, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (178, 34, 34, 255).
        /// </summary>
        public static Color32 Firebrick => new Color32(178, 34, 34, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 250, 240, 255).
        /// </summary>
        public static Color32 FloralWhite => new Color32(255, 250, 240, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (34, 139, 34, 255).
        /// </summary>
        public static Color32 ForestGreen => new Color32(34, 139, 34, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 0, 255, 255).
        /// </summary>
        public static Color32 Fuchsia => new Color32(255, 0, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (220, 220, 220, 255).
        /// </summary>
        public static Color32 Gainsboro => new Color32(220, 220, 220, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (248, 248, 255, 255).
        /// </summary>
        public static Color32 GhostWhite => new Color32(248, 248, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 215, 0, 255).
        /// </summary>
        public static Color32 Gold => new Color32(255, 215, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (218, 165, 32, 255).
        /// </summary>
        public static Color32 Goldenrod => new Color32(218, 165, 32, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (128, 128, 128, 255).
        /// </summary>
        public static Color32 Gray => new Color32(128, 128, 128, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 128, 0, 255).
        /// </summary>
        public static Color32 Green => new Color32(0, 128, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (173, 255, 47, 255).
        /// </summary>
        public static Color32 GreenYellow => new Color32(173, 255, 47, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (240, 255, 240, 255).
        /// </summary>
        public static Color32 Honeydew => new Color32(240, 255, 240, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 105, 180, 255).
        /// </summary>
        public static Color32 HotPink => new Color32(255, 105, 180, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (205, 92, 92, 255).
        /// </summary>
        public static Color32 IndianRed => new Color32(205, 92, 92, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (75, 0, 130, 255).
        /// </summary>
        public static Color32 Indigo => new Color32(75, 0, 130, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 255, 240, 255).
        /// </summary>
        public static Color32 Ivory => new Color32(255, 255, 240, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (240, 230, 140, 255).
        /// </summary>
        public static Color32 Khaki => new Color32(240, 230, 140, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (230, 230, 250, 255).
        /// </summary>
        public static Color32 Lavender => new Color32(230, 230, 250, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 240, 245, 255).
        /// </summary>
        public static Color32 LavenderBlush => new Color32(255, 240, 245, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (124, 252, 0, 255).
        /// </summary>
        public static Color32 LawnGreen => new Color32(124, 252, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 250, 205, 255).
        /// </summary>
        public static Color32 LemonChiffon => new Color32(255, 250, 205, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (173, 216, 230, 255).
        /// </summary>
        public static Color32 LightBlue => new Color32(173, 216, 230, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (240, 128, 128, 255).
        /// </summary>
        public static Color32 LightCoral => new Color32(240, 128, 128, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (224, 255, 255, 255).
        /// </summary>
        public static Color32 LightCyan => new Color32(224, 255, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (250, 250, 210, 255).
        /// </summary>
        public static Color32 LightGoldenrodYellow => new Color32(250, 250, 210, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (144, 238, 144, 255).
        /// </summary>
        public static Color32 LightGreen => new Color32(144, 238, 144, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (211, 211, 211, 255).
        /// </summary>
        public static Color32 LightGray => new Color32(211, 211, 211, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 182, 193, 255).
        /// </summary>
        public static Color32 LightPink => new Color32(255, 182, 193, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 160, 122, 255).
        /// </summary>
        public static Color32 LightSalmon => new Color32(255, 160, 122, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (32, 178, 170, 255).
        /// </summary>
        public static Color32 LightSeaGreen => new Color32(32, 178, 170, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (135, 206, 250, 255).
        /// </summary>
        public static Color32 LightSkyBlue => new Color32(135, 206, 250, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (119, 136, 153, 255).
        /// </summary>
        public static Color32 LightSlateGray => new Color32(119, 136, 153, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (176, 196, 222, 255).
        /// </summary>
        public static Color32 LightSteelBlue => new Color32(176, 196, 222, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 255, 224, 255).
        /// </summary>
        public static Color32 LightYellow => new Color32(255, 255, 224, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 255, 0, 255).
        /// </summary>
        public static Color32 Lime => new Color32(0, 255, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (50, 205, 50, 255).
        /// </summary>
        public static Color32 LimeGreen => new Color32(50, 205, 50, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (250, 240, 230, 255).
        /// </summary>
        public static Color32 Linen => new Color32(250, 240, 230, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 0, 255, 255).
        /// </summary>
        public static Color32 Magenta => new Color32(255, 0, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (128, 0, 0, 255).
        /// </summary>
        public static Color32 Maroon => new Color32(128, 0, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (102, 205, 170, 255).
        /// </summary>
        public static Color32 MediumAquamarine => new Color32(102, 205, 170, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 0, 205, 255).
        /// </summary>
        public static Color32 MediumBlue => new Color32(0, 0, 205, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (186, 85, 211, 255).
        /// </summary>
        public static Color32 MediumOrchid => new Color32(186, 85, 211, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (147, 112, 219, 255).
        /// </summary>
        public static Color32 MediumPurple => new Color32(147, 112, 219, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (60, 179, 113, 255).
        /// </summary>
        public static Color32 MediumSeaGreen => new Color32(60, 179, 113, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (123, 104, 238, 255).
        /// </summary>
        public static Color32 MediumSlateBlue => new Color32(123, 104, 238, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 250, 154, 255).
        /// </summary>
        public static Color32 MediumSpringGreen => new Color32(0, 250, 154, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (72, 209, 204, 255).
        /// </summary>
        public static Color32 MediumTurquoise => new Color32(72, 209, 204, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (199, 21, 133, 255).
        /// </summary>
        public static Color32 MediumVioletRed => new Color32(199, 21, 133, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (25, 25, 112, 255).
        /// </summary>
        public static Color32 MidnightBlue => new Color32(25, 25, 112, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (245, 255, 250, 255).
        /// </summary>
        public static Color32 MintCream => new Color32(245, 255, 250, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 228, 225, 255).
        /// </summary>
        public static Color32 MistyRose => new Color32(255, 228, 225, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 228, 181, 255).
        /// </summary>
        public static Color32 Moccasin => new Color32(255, 228, 181, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 222, 173, 255).
        /// </summary>
        public static Color32 NavajoWhite => new Color32(255, 222, 173, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 0, 128, 255).
        /// </summary>
        public static Color32 Navy => new Color32(0, 0, 128, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (253, 245, 230, 255).
        /// </summary>
        public static Color32 OldLace => new Color32(253, 245, 230, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (128, 128, 0, 255).
        /// </summary>
        public static Color32 Olive => new Color32(128, 128, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (107, 142, 35, 255).
        /// </summary>
        public static Color32 OliveDrab => new Color32(107, 142, 35, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 165, 0, 255).
        /// </summary>
        public static Color32 Orange => new Color32(255, 165, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 69, 0, 255).
        /// </summary>
        public static Color32 OrangeRed => new Color32(255, 69, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (218, 112, 214, 255).
        /// </summary>
        public static Color32 Orchid => new Color32(218, 112, 214, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (238, 232, 170, 255).
        /// </summary>
        public static Color32 PaleGoldenrod => new Color32(238, 232, 170, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (152, 251, 152, 255).
        /// </summary>
        public static Color32 PaleGreen => new Color32(152, 251, 152, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (175, 238, 238, 255).
        /// </summary>
        public static Color32 PaleTurquoise => new Color32(175, 238, 238, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (219, 112, 147, 255).
        /// </summary>
        public static Color32 PaleVioletRed => new Color32(219, 112, 147, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 239, 213, 255).
        /// </summary>
        public static Color32 PapayaWhip => new Color32(255, 239, 213, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 218, 185, 255).
        /// </summary>
        public static Color32 PeachPuff => new Color32(255, 218, 185, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (205, 133, 63, 255).
        /// </summary>
        public static Color32 Peru => new Color32(205, 133, 63, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 192, 203, 255).
        /// </summary>
        public static Color32 Pink => new Color32(255, 192, 203, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (221, 160, 221, 255).
        /// </summary>
        public static Color32 Plum => new Color32(221, 160, 221, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (176, 224, 230, 255).
        /// </summary>
        public static Color32 PowderBlue => new Color32(176, 224, 230, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (128, 0, 128, 255).
        /// </summary>
        public static Color32 Purple => new Color32(128, 0, 128, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 0, 0, 255).
        /// </summary>
        public static Color32 Red => new Color32(255, 0, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (188, 143, 143, 255).
        /// </summary>
        public static Color32 RosyBrown => new Color32(188, 143, 143, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (65, 105, 225, 255).
        /// </summary>
        public static Color32 RoyalBlue => new Color32(65, 105, 225, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (139, 69, 19, 255).
        /// </summary>
        public static Color32 SaddleBrown => new Color32(139, 69, 19, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (250, 128, 114, 255).
        /// </summary>
        public static Color32 Salmon => new Color32(250, 128, 114, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (244, 164, 96, 255).
        /// </summary>
        public static Color32 SandyBrown => new Color32(244, 164, 96, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (46, 139, 87, 255).
        /// </summary>
        public static Color32 SeaGreen => new Color32(46, 139, 87, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 245, 238, 255).
        /// </summary>
        public static Color32 SeaShell => new Color32(255, 245, 238, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (160, 82, 45, 255).
        /// </summary>
        public static Color32 Sienna => new Color32(160, 82, 45, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (192, 192, 192, 255).
        /// </summary>
        public static Color32 Silver => new Color32(192, 192, 192, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (135, 206, 235, 255).
        /// </summary>
        public static Color32 SkyBlue => new Color32(135, 206, 235, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (106, 90, 205, 255).
        /// </summary>
        public static Color32 SlateBlue => new Color32(106, 90, 205, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (112, 128, 144, 255).
        /// </summary>
        public static Color32 SlateGray => new Color32(112, 128, 144, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 250, 250, 255).
        /// </summary>
        public static Color32 Snow => new Color32(255, 250, 250, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 255, 127, 255).
        /// </summary>
        public static Color32 SpringGreen => new Color32(0, 255, 127, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (70, 130, 180, 255).
        /// </summary>
        public static Color32 SteelBlue => new Color32(70, 130, 180, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (210, 180, 140, 255).
        /// </summary>
        public static Color32 Tan => new Color32(210, 180, 140, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (0, 128, 128, 255).
        /// </summary>
        public static Color32 Teal => new Color32(0, 128, 128, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (216, 191, 216, 255).
        /// </summary>
        public static Color32 Thistle => new Color32(216, 191, 216, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 99, 71, 255).
        /// </summary>
        public static Color32 Tomato => new Color32(255, 99, 71, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (64, 224, 208, 255).
        /// </summary>
        public static Color32 Turquoise => new Color32(64, 224, 208, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (238, 130, 238, 255).
        /// </summary>
        public static Color32 Violet => new Color32(238, 130, 238, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (245, 222, 179, 255).
        /// </summary>
        public static Color32 Wheat => new Color32(245, 222, 179, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 255, 255, 255).
        /// </summary>
        public static Color32 White => new Color32(255, 255, 255, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (245, 245, 245, 255).
        /// </summary>
        public static Color32 WhiteSmoke => new Color32(245, 245, 245, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (255, 255, 0, 255).
        /// </summary>
        public static Color32 Yellow => new Color32(255, 255, 0, 255);

        /// <summary>
        /// Gets the system color with (R, G, B, A) = (154, 205, 50, 255).
        /// </summary>
        public static Color32 YellowGreen => new Color32(154, 205, 50, 255);

        /// <summary>
        /// Compares whether this Color32 structure is equal to the specified Color32.
        /// </summary>
        /// <param name="other">The Color32 structure to compare to.</param>
        /// <returns>True if both Color32 structures contain the same components; false otherwise.</returns>
        [Pure]
        public bool Equals(Color32 other)
        {
            return
                R == other.R &&
                G == other.G &&
                B == other.B &&
                A == other.A;
        }
    }
}
