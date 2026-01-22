using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Linq;
using System.Runtime.Loader;

namespace Himii
{
    public class ScriptManager
    {
        // 保持 ID 到 实例 的映射，用于运行时 Update (legacy support)
        // 但现在我们更倾向于让 C++ 持有 GCHandle
        private static Dictionary<ulong, Entity> _instances = new Dictionary<ulong, Entity>();
        private static Dictionary<string, Type> _entityClasses = new Dictionary<string, Type>();

        private class GameAssemblyLoadContext : AssemblyLoadContext
        {
            public GameAssemblyLoadContext() : base(isCollectible: true) { }

            protected override Assembly? Load(AssemblyName assemblyName)
            {
                var existingAssembly = AppDomain.CurrentDomain.GetAssemblies()
                    .FirstOrDefault(a => a.GetName().Name == assemblyName.Name);

                if (existingAssembly != null)
                    return existingAssembly;

                return null;
            }
        }

        private static GameAssemblyLoadContext? _currentContext;

        struct EntityState 
        { 
            public string ClassName; 
            public string Json; 
        }

        [UnmanagedCallersOnly]
        public static void LoadGameAssembly(IntPtr assemblyPathPtr)
        {
            string assemblyPath = Marshal.PtrToStringUTF8(assemblyPathPtr);
            Console.WriteLine($"[C#] Loading Assembly: {assemblyPath}");

            try
            {
                // 1. Snapshot State
                Dictionary<ulong, EntityState> savedStates = new Dictionary<ulong, EntityState>();
                if (_currentContext != null)
                {
                     var jsonOptions = new System.Text.Json.JsonSerializerOptions { IncludeFields = true };
                     foreach (var kvp in _instances)
                     {
                         try 
                         {
                             string json = System.Text.Json.JsonSerializer.Serialize(kvp.Value, kvp.Value.GetType(), jsonOptions);
                             string className = kvp.Value.GetType().FullName; // Get FullName
                             savedStates[kvp.Key] = new EntityState { ClassName = className, Json = json };
                         }
                         catch(Exception e)
                         {
                             Console.WriteLine($"[C#] Serialize Error during Reload: {e.Message}");
                         }
                     }
                     
                     _instances.Clear(); // Clear references to allow unloading
                     _entityClasses.Clear(); // Clear type cache
                     
                     _currentContext.Unload();
                     _currentContext = null;
                     GC.Collect(); 
                     GC.WaitForPendingFinalizers();
                }

                // 2. Load New Assembly
                _currentContext = new GameAssemblyLoadContext();
                
                // Generate PDB path from assembly path
                string pdbPath = Path.ChangeExtension(assemblyPath, ".pdb");
                bool pdbExists = File.Exists(pdbPath);

                try {
                    using (var stream = new FileStream(assemblyPath, FileMode.Open, FileAccess.Read, FileShare.Read))
                    {
                        Assembly gameAssembly;
                        if (pdbExists)
                        {
                            using (var pdbStream = new FileStream(pdbPath, FileMode.Open, FileAccess.Read, FileShare.Read))
                            {
                                Console.WriteLine($"[C#] Loading PDB: {pdbPath}");
                                gameAssembly = _currentContext.LoadFromStream(stream, pdbStream);
                            }
                        }
                        else
                        {
                            gameAssembly = _currentContext.LoadFromStream(stream);
                        }
                        
                        LoadAssemblyClasses(gameAssembly);
                    }
                }
                catch(Exception e)
                {
                    Console.WriteLine($"[C#] Load Assembly Error: {e.Message}");
                    return;
                }
                
                // 3. Restore State
                if (savedStates.Count > 0)
                {
                    var jsonOptions = new System.Text.Json.JsonSerializerOptions { IncludeFields = true };
                    foreach (var kvp in savedStates)
                    {
                         ulong entityID = kvp.Key;
                         EntityState state = kvp.Value;
                         
                         // Instantiate properly sets up internal map
                         Entity newEntity = InstantiateClassInternal(entityID, state.ClassName);
                         
                         if (newEntity != null)
                         {
                             try
                             {
                                 // Deserialize fields
                                 // We deserialize to a temp object then copy
                                 object tempObj = System.Text.Json.JsonSerializer.Deserialize(state.Json, newEntity.GetType(), jsonOptions);
                                 
                                 foreach (var field in newEntity.GetType().GetFields(BindingFlags.Public | BindingFlags.Instance))
                                 {
                                     field.SetValue(newEntity, field.GetValue(tempObj));
                                 }
                             }
                             catch(Exception e)
                             {
                                 Console.WriteLine($"[C#] Restore State Error for {state.ClassName}: {e.Message}");
                             }
                         }
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C#] Error loading assembly: {e.Message}");
                Console.WriteLine(e.StackTrace);
            }
        }

        private static void LoadAssemblyClasses(Assembly assembly)
        {
            _entityClasses.Clear();
            _instances.Clear();

            foreach (var type in assembly.GetTypes())
            {
                if (type.IsAbstract || type.IsInterface)
                    continue;

                if (type.IsSubclassOf(typeof(Entity)))
                {
                    string fullName = type.FullName;
                    _entityClasses[fullName] = type;
                    if (type.Name != fullName)
                        _entityClasses[type.Name] = type;
                    Console.WriteLine($"[C#] Registered Entity Class: {fullName}");
                }
            }
        }

        [UnmanagedCallersOnly]
        public static int EntityClassExists(IntPtr classNamePtr)
        {
            try
            {
                if (classNamePtr == IntPtr.Zero) return 0;
                string className = Marshal.PtrToStringUTF8(classNamePtr);
                if (string.IsNullOrEmpty(className)) return 0;

                return _entityClasses.ContainsKey(className) ? 1 : 0;
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] EntityClassExists failed: {e.Message}");
                return 0;
            }
        }

        // [NEW] 内部实现，供 C# 互相调用
        private static Entity InstantiateClassInternal(ulong entityID, string className)
        {
            try
            {
                if (_entityClasses.TryGetValue(className, out Type type))
                {
                    // 这里最容易报错：如果你的 Entity 构造函数里写了有问题的代码
                    var entity = (Entity)Activator.CreateInstance(type);
                    entity.ID = entityID;
                    _instances[entityID] = entity;

                    // 如果是在编辑器模式下，可能不应该调用 OnCreate？
                    // 或者你的 OnCreate 里用到了 Input，但编辑器模式下 Input 可能未初始化
                    // 建议用 try catch 包裹 OnCreate
                    try
                    {
                        entity.OnCreate();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"[C# Error] {className}.OnCreate threw exception: {ex.Message}");
                    }

                    return entity;
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] InstantiateClassInternal failed: {e.Message}");
            }
            return null;
        }

        // [NEW] 创建对象并返回 GCHandle 的 IntPtr
        [UnmanagedCallersOnly]
        public static IntPtr InstantiateClass(ulong entityID, IntPtr classNamePtr)
        {
            try
            {
                string className = Marshal.PtrToStringUTF8(classNamePtr);

                // 增加双重检查，防止空引用
                if (!_entityClasses.ContainsKey(className))
                {
                    Console.WriteLine($"[C# Error] Cannot instantiate class '{className}': Class not found in dictionary.");
                    return IntPtr.Zero;
                }

                // 调用内部实例化逻辑
                var entity = InstantiateClassInternal(entityID, className);

                if (entity != null)
                {
                    GCHandle handle = GCHandle.Alloc(entity);
                    return GCHandle.ToIntPtr(handle);
                }
            }
            catch (Exception e)
            {
                // 捕获构造函数里的错误！
                Console.WriteLine($"[C# Critical Error] Failed to Instantiate '{Marshal.PtrToStringUTF8(classNamePtr)}': {e.Message}");
                Console.WriteLine(e.StackTrace);
            }

            return IntPtr.Zero;
        }
        
        // [NEW] 释放 GCHandle
        [UnmanagedCallersOnly]
        public static void ReleaseInstance(IntPtr handlePtr)
        {
            if (handlePtr == IntPtr.Zero) return;
            GCHandle handle = GCHandle.FromIntPtr(handlePtr);
            if (handle.IsAllocated)
            {
                handle.Free();
            }
        }

        [UnmanagedCallersOnly]
        public static void OnCreateEntity(ulong entityID, IntPtr classNamePtr)
        {
             // 旧接口保留，但不返回 Handle
             string className = Marshal.PtrToStringUTF8(classNamePtr);
             InstantiateClassInternal(entityID, className);
        }

        [UnmanagedCallersOnly]
        public static void OnUpdateEntity(ulong entityID, float ts)
        {
            try
            {
                if (_instances.TryGetValue(entityID, out var entity))
                {
                    entity.OnUpdate(ts);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] OnUpdate failed: {e.Message}");
            }
        }
    }
}