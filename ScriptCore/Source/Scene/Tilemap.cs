using System;

namespace Himii
{
    public class Tilemap : Component
    {
        public uint Width
        {
            get 
            {
                 InternalCalls.Tilemap_GetSize(Entity.ID, out uint w, out uint h);
                 return w;
            }
            set
            {
                 InternalCalls.Tilemap_GetSize(Entity.ID, out uint w, out uint h);
                 InternalCalls.Tilemap_SetSize(Entity.ID, value, h);
            }
        }
        
        public uint Height
        {
             get
             {
                 InternalCalls.Tilemap_GetSize(Entity.ID, out uint w, out uint h);
                 return h;
             }
             set
             {
                 InternalCalls.Tilemap_GetSize(Entity.ID, out uint w, out uint h);
                 InternalCalls.Tilemap_SetSize(Entity.ID, w, value);
             }
        }

        public ushort GetTile(uint x, uint y)
        {
             return InternalCalls.Tilemap_GetTile(Entity.ID, x, y);
        }

        public void SetTile(uint x, uint y, ushort tileID)
        {
             InternalCalls.Tilemap_SetTile(Entity.ID, x, y, tileID);
        }
    }
}
