using System;
using System.Reflection;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Himii
{
    public static class ReflectionBridge
    {
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

                FieldInfo[] fields = type.GetFields(BindingFlags.Public | BindingFlags.Instance);

                List<string> fieldList = new List<string>();
                foreach (var field in fields)
                {
                    // 过滤掉不支持的类型，防止后续 GetValue 崩溃
                    // 或者全部返回，让 C++ 决定是否显示
                    fieldList.Add($"{field.Name}:{field.FieldType.Name}");
                }

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
            FieldInfo field = instance.GetType().GetField(fieldName);

            if (field != null && field.FieldType == typeof(T))
            {
                value = (T)field.GetValue(instance);
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
            FieldInfo field = instance.GetType().GetField(fieldName);

            if (field != null && field.FieldType == typeof(T))
            {
                field.SetValue(instance, value);
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
                var fields = type.GetFields(BindingFlags.Public | BindingFlags.Instance);
                
                Dictionary<string, object> data = new Dictionary<string, object>();

                foreach (var field in fields)
                {
                    // Filter: Only serialize ScriptFields that we support in Editor (or are serializable)
                    // For simplicity, we serialize all public fields, assuming they are data.
                    // Special handling for Entity reference to avoid cycle/deep serialization
                    if (field.FieldType == typeof(Entity))
                    {
                        var entityRef = field.GetValue(instance) as Entity;
                        data[field.Name] = entityRef != null ? entityRef.ID : 0;
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
                var fields = type.GetFields(BindingFlags.Public | BindingFlags.Instance);

                foreach (var field in fields)
                {
                    if (data.TryGetValue(field.Name, out System.Text.Json.JsonElement element))
                    {
                        try
                        {
                            if (field.FieldType == typeof(Entity))
                            {
                                ulong id = element.GetUInt64();
                                if (id > 0)
                                    field.SetValue(instance, new Entity(id));
                                else
                                    field.SetValue(instance, null);
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
