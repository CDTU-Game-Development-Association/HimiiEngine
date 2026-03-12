using System;

namespace Himii
{
    internal static class HashUtils
    {
        // 稳定的 32-bit FNV-1a 哈希：跨进程/跨平台结果一致
        // 面试点：为什么不用 string.GetHashCode？因为它在 .NET 中可能被随机化，无法做跨语言稳定映射
        internal static int Fnv1A32(string text)
        {
            if (string.IsNullOrEmpty(text))
                return 0;

            unchecked
            {
                const uint FnvOffsetBasis = 2166136261u;
                const uint FnvPrime = 16777619u;

                uint hash = FnvOffsetBasis;
                for (int i = 0; i < text.Length; i++)
                {
                    hash ^= text[i];
                    hash *= FnvPrime;
                }
                return (int)hash;
            }
        }
    }
}

