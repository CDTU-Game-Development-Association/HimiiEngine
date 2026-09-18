using System;
using System.Reflection;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace HimiiEngine
{
    public static class ReflectionBridge
    {
        private static FieldInfo GetInstanceField(Type type, string fieldName)
        {
            return type.GetField(fieldName,
                BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        }

        private static bool IsMarkedWithSerializeField(FieldInfo field)
        {
            // typeof(SerializeField) 在 GameAssembly 与 ScriptCore 分属不同 ALC 时可能失败，
            // 因此用 CustomAttributeData 的 FullName 做跨上下文匹配。
            if (field.IsDefined(typeof(SerializeField), inherit: true))
                return true;

            foreach (CustomAttributeData attributeData in field.CustomAttributes)
            {
                string attributeFullName = attributeData.AttributeType.FullName ?? string.Empty;
                if (attributeFullName == "HimiiEngine.SerializeField")
                    return true;
            }

            return false;
        }

        private static bool ShouldSerializeField(FieldInfo field)
        {
            if (field.IsStatic || field.IsInitOnly)
                return false;

            if (field.IsDefined(typeof(NonSerializedAttribute), inherit: true))
                return false;

            if (field.IsPublic)
                return true;

            return IsMarkedWithSerializeField(field);
        }

        private static IEnumerable<FieldInfo> GetSerializedFields(Type type)
        {
            foreach (FieldInfo field in type.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance))
            {
                if (ShouldSerializeField(field))
                    yield return field;
            }
        }

        private static bool IsEntityOrButtonField(FieldInfo field)
        {
            return field.FieldType == typeof(Entity) || field.FieldType == typeof(Button);
        }

        private static ulong ReadReferencedEntityIdentifier(FieldInfo field, object instance)
        {
            object? fieldValue = field.GetValue(instance);
            if (fieldValue is Entity entityReference)
                return entityReference.ID;
            if (fieldValue is Button buttonComponent)
            {
                Entity? referencedEntity = buttonComponent.Entity;
                return referencedEntity != null ? referencedEntity.ID : 0UL;
            }
            return 0UL;
        }

        private static void WriteReferencedEntityIdentifier(FieldInfo field, object instance, ulong entityIdentifier)
        {
            if (field.FieldType == typeof(Entity))
            {
                if (entityIdentifier > 0)
                    field.SetValue(instance, new Entity(entityIdentifier));
                else
                    field.SetValue(instance, null);
                return;
            }

            if (field.FieldType == typeof(Button))
            {
                if (entityIdentifier > 0)
                    field.SetValue(instance, new Button { Entity = new Entity(entityIdentifier) });
                else
                    field.SetValue(instance, null);
            }
        }

        [UnmanagedCallersOnly]
        public static IntPtr GetFields(IntPtr instanceHandle)
        {
            try
            {
                if (instanceHandle == IntPtr.Zero) return IntPtr.Zero;

                GCHandle handle = GCHandle.FromIntPtr(instanceHandle);
                if (!handle.IsAllocated || handle.Target == null) return IntPtr.Zero;

                object instance = handle.Target;
                Type type = instance.GetType();

                List<string> fieldList = new List<string>();
                foreach (FieldInfo field in GetSerializedFields(type))
                    fieldList.Add($"{field.Name}:{field.FieldType.Name}");

                string result = string.Join(";", fieldList);
                return Marshal.StringToCoTaskMemUTF8(result);
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] GetFields failed: {e.Message}");
                return IntPtr.Zero;
            }
        }
        
        [UnmanagedCallersOnly]
        public static unsafe int GetFloat(IntPtr instanceHandle, IntPtr fieldNamePtr, float* outValue)
        {
            return GetValueT(instanceHandle, fieldNamePtr, out *outValue) ? 1 : 0;
        }

        [UnmanagedCallersOnly]
        public static void SetFloat(IntPtr instanceHandle, IntPtr fieldNamePtr, float value)
        {
            SetValueT(instanceHandle, fieldNamePtr, value);
        }

        // --- Int ---
        [UnmanagedCallersOnly]
        public static unsafe int GetInt(IntPtr instanceHandle, IntPtr fieldNamePtr, int* outValue)
        {
            return GetValueT(instanceHandle, fieldNamePtr, out *outValue) ? 1 : 0;
        }

        [UnmanagedCallersOnly]
        public static void SetInt(IntPtr instanceHandle, IntPtr fieldNamePtr, int value)
        {
            SetValueT(instanceHandle, fieldNamePtr, value);
        }

        // --- Bool ---
        [UnmanagedCallersOnly]
        public static unsafe int GetBool(IntPtr instanceHandle, IntPtr fieldNamePtr, int* outValue)
        {
            // C++ bool 通常是 1 字节，但为了互操作安全，通常用 int (4字节) 传输
            bool val;
            bool res = GetValueT(instanceHandle, fieldNamePtr, out val);
            *outValue = val ? 1 : 0;
            return res ? 1 : 0;
        }

        [UnmanagedCallersOnly]
        public static void SetBool(IntPtr instanceHandle, IntPtr fieldNamePtr, int value)
        {
            SetValueT(instanceHandle, fieldNamePtr, value != 0);
        }

        // --- Vector2 ---
        [UnmanagedCallersOnly]
        public static unsafe int GetVector2(IntPtr instanceHandle, IntPtr fieldNamePtr, Vector2* outValue)
        {
            return GetValueT(instanceHandle, fieldNamePtr, out *outValue) ? 1 : 0;
        }

        [UnmanagedCallersOnly]
        public static unsafe void SetVector2(IntPtr instanceHandle, IntPtr fieldNamePtr, Vector2* value)
        {
            SetValueT(instanceHandle, fieldNamePtr, *value);
        }

        // --- Vector3 ---
        [UnmanagedCallersOnly]
        public static unsafe int GetVector3(IntPtr instanceHandle, IntPtr fieldNamePtr, Vector3* outValue)
        {
            return GetValueT(instanceHandle, fieldNamePtr, out *outValue) ? 1 : 0;
        }

        [UnmanagedCallersOnly]
        public static unsafe void SetVector3(IntPtr instanceHandle, IntPtr fieldNamePtr, Vector3* value)
        {
            SetValueT(instanceHandle, fieldNamePtr, *value);
        }

        // --- Vector4 ---
        [UnmanagedCallersOnly]
        public static unsafe int GetVector4(IntPtr instanceHandle, IntPtr fieldNamePtr, Vector4* outValue)
        {
            return GetValueT(instanceHandle, fieldNamePtr, out *outValue) ? 1 : 0;
        }

        [UnmanagedCallersOnly]
        public static unsafe void SetVector4(IntPtr instanceHandle, IntPtr fieldNamePtr, Vector4* value)
        {
            SetValueT(instanceHandle, fieldNamePtr, *value);
        }

        private static bool GetValueT<T>(IntPtr instanceHandle, IntPtr fieldNamePtr, out T value)
        {
            value = default;
            GCHandle handle = GCHandle.FromIntPtr(instanceHandle);
            object instance = handle.Target;
            if (instance == null) return false;

            string fieldName = Marshal.PtrToStringUTF8(fieldNamePtr);
            FieldInfo field = GetInstanceField(instance.GetType(), fieldName);

            if (field == null)
                return false;

            // 1. 精确类型匹配（float、int、bool、Vector 等）
            if (field.FieldType == typeof(T))
            {
                value = (T)field.GetValue(instance);
                return true;
            }

            // 2. 枚举类型：允许用 int 在 C++ 侧读写 enum（用于 KeyCode 等）
            if (field.FieldType.IsEnum && typeof(T) == typeof(int))
            {
                int raw = Convert.ToInt32(field.GetValue(instance));
                object boxed = raw;
                value = (T)boxed;
                return true;
            }

            return false;
        }

        private static void SetValueT<T>(IntPtr instanceHandle, IntPtr fieldNamePtr, T value)
        {
            GCHandle handle = GCHandle.FromIntPtr(instanceHandle);
            object instance = handle.Target;
            if (instance == null) return;

            string fieldName = Marshal.PtrToStringUTF8(fieldNamePtr);
            FieldInfo field = GetInstanceField(instance.GetType(), fieldName);

            if (field == null)
                return;

            // 1. 精确类型匹配
            if (field.FieldType == typeof(T))
            {
                field.SetValue(instance, value);
                return;
            }

            // 2. 枚举：允许通过 int 写入 enum（KeyCode）
            if (field.FieldType.IsEnum && typeof(T) == typeof(int))
            {
                int raw = (int)(object)value;
                object enumValue = Enum.ToObject(field.FieldType, raw);
                field.SetValue(instance, enumValue);
            }
        }

        [UnmanagedCallersOnly]
        public static IntPtr Serialize(IntPtr instanceHandle)
        {
            try
            {
                GCHandle handle = GCHandle.FromIntPtr(instanceHandle);
                object instance = handle.Target;
                if (instance == null) return IntPtr.Zero;

                Type type = instance.GetType();
                Dictionary<string, object> data = new Dictionary<string, object>();

                foreach (FieldInfo field in GetSerializedFields(type))
                {
                    // Filter: Only serialize ScriptFields that we support in Editor (or are serializable)
                    // For simplicity, we serialize all public fields, assuming they are data.
                    // Special handling for Entity / Button references to avoid cycle/deep serialization
                    if (IsEntityOrButtonField(field))
                    {
                        data[field.Name] = ReadReferencedEntityIdentifier(field, instance);
                    }
                    else
                    {
                        // Primitives, Vectors, etc.
                        data[field.Name] = field.GetValue(instance);
                    }
                }

                // Serialize the dictionary (flat structure)
                string json = System.Text.Json.JsonSerializer.Serialize(data);
                return Marshal.StringToCoTaskMemUTF8(json);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[C#] Serialize Error: {ex.Message}");
                return IntPtr.Zero;
            }
        }

        // --- String ---
        [UnmanagedCallersOnly]
        public static IntPtr GetString(IntPtr instanceHandle, IntPtr fieldNamePtr)
        {
            try
            {
                GCHandle handle = GCHandle.FromIntPtr(instanceHandle);
                object instance = handle.Target;
                if (instance == null) return IntPtr.Zero;

                string fieldName = Marshal.PtrToStringUTF8(fieldNamePtr);
                FieldInfo field = GetInstanceField(instance.GetType(), fieldName);
                if (field == null || field.FieldType != typeof(string))
                    return IntPtr.Zero;

                string value = (string)field.GetValue(instance) ?? string.Empty;
                return Marshal.StringToCoTaskMemUTF8(value);
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] GetString failed: {e.Message}");
                return IntPtr.Zero;
            }
        }

        [UnmanagedCallersOnly]
        public static unsafe int GetEntityID(IntPtr instanceHandle, IntPtr fieldNamePtr, ulong* outValue)
        {
            try
            {
                if (instanceHandle == IntPtr.Zero || fieldNamePtr == IntPtr.Zero || outValue == null)
                    return 0;

                GCHandle handle = GCHandle.FromIntPtr(instanceHandle);
                object instance = handle.Target;
                if (instance == null) return 0;

                string fieldName = Marshal.PtrToStringUTF8(fieldNamePtr);
                FieldInfo field = GetInstanceField(instance.GetType(), fieldName);
                if (field == null || !IsEntityOrButtonField(field))
                    return 0;

                *outValue = ReadReferencedEntityIdentifier(field, instance);
                return 1;
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] GetEntityID failed: {e.Message}");
                return 0;
            }
        }

        [UnmanagedCallersOnly]
        public static void SetEntityID(IntPtr instanceHandle, IntPtr fieldNamePtr, ulong entityID)
        {
            try
            {
                if (instanceHandle == IntPtr.Zero || fieldNamePtr == IntPtr.Zero)
                    return;

                GCHandle handle = GCHandle.FromIntPtr(instanceHandle);
                object instance = handle.Target;
                if (instance == null) return;

                string fieldName = Marshal.PtrToStringUTF8(fieldNamePtr);
                FieldInfo field = GetInstanceField(instance.GetType(), fieldName);
                if (field == null || !IsEntityOrButtonField(field))
                    return;

                WriteReferencedEntityIdentifier(field, instance, entityID);
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] SetEntityID failed: {e.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static void SetString(IntPtr instanceHandle, IntPtr fieldNamePtr, IntPtr valuePtr)
        {
            try
            {
                GCHandle handle = GCHandle.FromIntPtr(instanceHandle);
                object instance = handle.Target;
                if (instance == null) return;

                string fieldName = Marshal.PtrToStringUTF8(fieldNamePtr);
                string value = Marshal.PtrToStringUTF8(valuePtr) ?? string.Empty;

                FieldInfo field = GetInstanceField(instance.GetType(), fieldName);
                if (field != null && field.FieldType == typeof(string))
                {
                    field.SetValue(instance, value);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] SetString failed: {e.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static void Deserialize(IntPtr instanceHandle, IntPtr jsonPtr)
        {
            try
            {
                GCHandle handle = GCHandle.FromIntPtr(instanceHandle);
                object instance = handle.Target;
                if (instance == null) return;

                string json = Marshal.PtrToStringUTF8(jsonPtr);
                if (string.IsNullOrEmpty(json)) return;

                // Deserialize into Dictionary<string, JsonElement> to handle mixed types
                var data = System.Text.Json.JsonSerializer.Deserialize<Dictionary<string, System.Text.Json.JsonElement>>(json);
                if (data == null) return;

                Type type = instance.GetType();

                foreach (FieldInfo field in GetSerializedFields(type))
                {
                    if (data.TryGetValue(field.Name, out System.Text.Json.JsonElement element))
                    {
                        try
                        {
                            if (IsEntityOrButtonField(field))
                            {
                                ulong identifier = element.GetUInt64();
                                WriteReferencedEntityIdentifier(field, instance, identifier);
                            }
                            else
                            {
                                // Convert JsonElement to target type
                                object value = System.Text.Json.JsonSerializer.Deserialize(element.GetRawText(), field.FieldType);
                                field.SetValue(instance, value);
                            }
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine($"[C#] Failed to deserialize field '{field.Name}': {ex.Message}");
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C#] Deserialize Error: {e.Message}");
            }
        }
    }
}
