namespace ProjectWS.FileFormats.Extensions
{
    public class ByteArrayComparer : IEqualityComparer<byte[]>
    {
        private static readonly EqualityComparer<byte> elementComparer = EqualityComparer<byte>.Default;

        public bool Equals(byte[] left, byte[] right)
        {
            /*
            if (left == null || right == null)
                return left == right;

            if (left.Length != right.Length)
                return false;

            for (int i = 0; i < left.Length; i++)
            {
                if (left[i] != right[i])
                    return false;
            }

            return true;
            */

            return UnsafeCompare(left, right);
        }

        // Copyright (c) 2008-2013 Hafthor Stefansson
        // Distributed under the MIT/X11 software license
        // Ref: http://www.opensource.org/licenses/mit-license.php.
        static unsafe bool UnsafeCompare(byte[] a1, byte[] a2)
        {
            if (a1 == a2) return true;
            if (a1 == null || a2 == null || a1.Length != a2.Length)
                return false;
            fixed (byte* p1 = a1, p2 = a2)
            {
                byte* x1 = p1, x2 = p2;
                int l = a1.Length;
                for (int i = 0; i < l / 8; i++, x1 += 8, x2 += 8)
                    if (*((long*)x1) != *((long*)x2)) return false;
                if ((l & 4) != 0) { if (*((int*)x1) != *((int*)x2)) return false; x1 += 4; x2 += 4; }
                if ((l & 2) != 0) { if (*((short*)x1) != *((short*)x2)) return false; x1 += 2; x2 += 2; }
                if ((l & 1) != 0) if (*((byte*)x1) != *((byte*)x2)) return false;
                return true;
            }
        }

        /*
        public int GetHashCode(byte[] key)
        {
            if (key == null)
                throw new ArgumentNullException("key");

            int sum = 0;
            foreach (byte cur in key)
            {
                sum += cur;
            }

            return sum;
        }
        */
        /*
        public int GetHashCode(byte[] array)
        {
            unchecked
            {
                if (array == null)
                {
                    return 0;
                }
                int hash = 17;
                foreach (byte element in array)
                {
                    hash = hash * 31 + elementComparer.GetHashCode(element);
                }
                return hash;
            }
        }
        */
        public int GetHashCode(byte[] key)
        {
            return new System.Numerics.BigInteger(key).GetHashCode();
        }
    }
}