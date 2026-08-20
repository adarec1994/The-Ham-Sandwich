using System;

namespace WildStar.Model;

public static class HalfBits
{
    public static float ToSingle(ushort half)
    {
        uint magnitude = (uint)half & 0x7FFFu;
        uint sign = ((uint)half & 0x8000u) << 16;

        if ((magnitude & 0x7C00u) != 0)
        {
            return BitConverter.UInt32BitsToSingle(sign | ((magnitude + 114688u) << 13));
        }

        if ((magnitude & 0x3FFu) != 0)
        {
            uint mantissa = (magnitude & 0x3FFu) << 13;
            int exponent = 113;

            while (mantissa <= 0x7FFFFFu)
            {
                mantissa *= 2;
                exponent--;
            }

            return BitConverter.UInt32BitsToSingle(
                sign | ((uint)exponent << 23) | (mantissa & 0x7FFFFFu));
        }

        return BitConverter.UInt32BitsToSingle(sign | magnitude);
    }
}
