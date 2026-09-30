using System;
using System.Runtime.InteropServices;

namespace HimiiEngine
{
    public class Entity
    {
        public ulong ID { get; internal set; }

        protected Entity() { ID = 0; }

        internal Entity(ulong id) { ID = id; }

        public static Entity Create(string name)
        {
            IntPtr namePtr = Marshal.StringToCoTaskMemUTF8(name);
            ulong id = InternalCalls.Scene_CreateEntity(namePtr);
            Marshal.FreeCoTaskMem(namePtr);
            return new Entity(id);
        }

        public static void Destroy(Entity entity)
        {
            if (entity != null)
                InternalCalls.Scene_DestroyEntity(entity.ID);
        }

        public static Entity Find(string name)
        {
            IntPtr namePtr = Marshal.StringToCoTaskMemUTF8(name);
            ulong id = InternalCalls.Scene_FindEntityByName(namePtr);
            Marshal.FreeCoTaskMem(namePtr);

            if (id == 0) return null;
            return new Entity(id);
        }

        /// <summary>
        /// 从 Prefab 资产实例化整棵实体子树（路径相对于 assets，例如 prefabs/Enemy.hprefab）。
        /// 返回根实体，根没有父节点。要挂到某个实体下，再调用 SetParent(parent, false)。
        /// </summary>
        public static Entity Instantiate(string prefabPath)
        {
            if (string.IsNullOrEmpty(prefabPath) || InternalCalls.Scene_InstantiatePrefab == null)
                return null;

            IntPtr prefabPathPointer = Marshal.StringToCoTaskMemUTF8(prefabPath);
            try
            {
                ulong entityIdentifier = InternalCalls.Scene_InstantiatePrefab(prefabPathPointer);
                return entityIdentifier == 0 ? null : new Entity(entityIdentifier);
            }
            finally
            {
                Marshal.FreeCoTaskMem(prefabPathPointer);
            }
        }

        // --- Component Accessors ---

        public Vector3 Position
        {
            get
            {
                InternalCalls.Transform_GetTranslation(ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetTranslation(ID, ref value);
        }

        // 缓存 Transform 实例，避免每次访问 entity.Transform 都产生垃圾回收 (GC)
        private Transform _transform;

        public Transform Transform
        {
            get
            {
                if (_transform == null)
                {
                    _transform = new Transform();
                    _transform.Entity = this;
                }
                return _transform;
            }
        }

        public Entity Parent
        {
            get
            {
                if (InternalCalls.Entity_GetParent == null)
                    return null;
                ulong parentIdentifier = InternalCalls.Entity_GetParent(ID);
                return parentIdentifier == 0 ? null : new Entity(parentIdentifier);
            }
            set => SetParent(value, true);
        }

        public void SetParent(Entity parent, bool keepWorldPosition = true)
        {
            if (InternalCalls.Entity_SetParent == null)
                return;

            ulong parentIdentifier = parent != null ? parent.ID : 0;
            InternalCalls.Entity_SetParent(ID, parentIdentifier, keepWorldPosition ? (byte)1 : (byte)0);
        }

        public Entity[] GetChildren()
        {
            if (InternalCalls.Entity_GetChildCount == null || InternalCalls.Entity_GetChildAt == null)
                return Array.Empty<Entity>();

            int childCount = InternalCalls.Entity_GetChildCount(ID);
            Entity[] children = new Entity[childCount];
            for (int childIndex = 0; childIndex < childCount; ++childIndex)
            {
                ulong childIdentifier = InternalCalls.Entity_GetChildAt(ID, childIndex);
                children[childIndex] = childIdentifier == 0 ? null : new Entity(childIdentifier);
            }

            return children;
        }



        public bool HasComponent<T>() where T : Component, new()
        {
            // 不要用 string.GetHashCode：.NET 的字符串哈希在不同进程/不同运行中可能不同（安全随机化）
            // 我们用稳定的 FNV-1a 32-bit，把类型全名映射成跨语言一致的 ID
            int typeId = HashUtils.Fnv1A32(typeof(T).FullName);
            return InternalCalls.Entity_HasComponent(ID, typeId) != 0;
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if (!HasComponent<T>())
                return null;

            T component = new T();
            component.Entity = this;
            return component;
        }

        public virtual void OnCreate() { }
        public virtual void OnUpdate(float timestep) { }
        /// <summary>物理步进与碰撞事件之后调用，适合移动、跳跃与着地检测。</summary>
        public virtual void OnFixedUpdate(float timestep) { }
        public virtual void OnDestroy() { }

        public virtual void OnCollisionEnter2D(Collision2DInfo collision) { }
        public virtual void OnCollisionExit2D(Collision2DInfo collision) { }
        public virtual void OnTriggerEnter2D(Collision2DInfo collision) { }
        public virtual void OnTriggerExit2D(Collision2DInfo collision) { }

        public virtual void OnPointerEnter() { }
        public virtual void OnPointerExit() { }
        public virtual void OnPointerDown() { }
        public virtual void OnPointerUp() { }
        public virtual void OnPointerClick() { }
    }
}
