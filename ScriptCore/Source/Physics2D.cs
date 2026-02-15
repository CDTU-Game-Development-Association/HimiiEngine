using System;

namespace Himii
{
    public static class Physics2D
    {
        public static RaycastHit2D Raycast(Vector2 start, Vector2 end)
        {
            InternalCalls.Physics2D_Raycast(ref start, ref end, out RaycastHit2D hit);
            return hit;
        }
    }
}
