using System.Runtime.InteropServices;

namespace HimiiEngine
{
    public abstract class Component
    {
        public Entity Entity { get; internal set; }
    }

    public class Transform : Component
    {
        public Vector3 Position
        {
            get
            {
                InternalCalls.Transform_GetTranslation(Entity.ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetTranslation(Entity.ID, ref value);
        }

        public Vector3 Rotation
        {
            get
            {
                InternalCalls.Transform_GetRotation(Entity.ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetRotation(Entity.ID, ref value);
        }

        public Vector3 Scale
        {
            get
            {
                InternalCalls.Transform_GetScale(Entity.ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetScale(Entity.ID, ref value);
        }

        public Vector3 WorldPosition
        {
            get
            {
                InternalCalls.Transform_GetWorldTranslation(Entity.ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetWorldTranslation(Entity.ID, ref value);
        }

        public Vector3 WorldRotation
        {
            get
            {
                InternalCalls.Transform_GetWorldRotation(Entity.ID, out Vector3 result);
                return result;
            }
        }
    }

    public class UIText : Component
    {
        public string Text
        {
            get
            {
                if (InternalCalls.UIText_GetText == null)
                    return string.Empty;
                IntPtr textPointer = InternalCalls.UIText_GetText(Entity.ID);
                return textPointer == IntPtr.Zero
                    ? string.Empty
                    : Marshal.PtrToStringUTF8(textPointer) ?? string.Empty;
            }
            set
            {
                if (InternalCalls.UIText_SetText == null)
                    return;
                IntPtr textPointer = Marshal.StringToCoTaskMemUTF8(value ?? string.Empty);
                InternalCalls.UIText_SetText(Entity.ID, textPointer);
                Marshal.FreeCoTaskMem(textPointer);
            }
        }

        public Vector4 Color
        {
            get
            {
                Vector4 result = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);
                InternalCalls.UIText_GetColor?.Invoke(Entity.ID, out result);
                return result;
            }
            set => InternalCalls.UIText_SetColor?.Invoke(Entity.ID, ref value);
        }

        public float FontSize
        {
            get => InternalCalls.UIText_GetFontSize?.Invoke(Entity.ID) ?? 48.0f;
            set => InternalCalls.UIText_SetFontSize?.Invoke(Entity.ID, value);
        }
    }

    public class Button : Component
    {
        private static readonly Dictionary<ulong, ButtonClickedEvent> ClickedEventsByEntityIdentifier =
            new Dictionary<ulong, ButtonClickedEvent>();

        public bool Interactable
        {
            get => InternalCalls.UIButton_GetInteractable?.Invoke(Entity.ID) != 0;
            set => InternalCalls.UIButton_SetInteractable?.Invoke(Entity.ID, (byte)(value ? 1 : 0));
        }

        public bool IsPointerInside =>
            InternalCalls.UIButton_GetIsPointerInside?.Invoke(Entity.ID) != 0;

        public bool IsPressed =>
            InternalCalls.UIButton_GetIsPressed?.Invoke(Entity.ID) != 0;

        public bool WasClickedThisFrame =>
            InternalCalls.UIButton_GetWasClickedThisFrame?.Invoke(Entity.ID) != 0;

        public ButtonClickedEvent OnClick => GetOrCreateClickedEvent(Entity.ID);

        public sealed class ButtonClickedEvent
        {
            private readonly List<Action> listeners = new List<Action>();

            internal ButtonClickedEvent()
            {
            }

            public void AddListener(Action? action)
            {
                if (action == null)
                    return;
                listeners.Add(action);
            }

            public void RemoveListener(Action? action)
            {
                if (action == null)
                    return;
                listeners.Remove(action);
            }

            public void RemoveAllListeners()
            {
                listeners.Clear();
            }

            internal void InvokeListeners()
            {
                if (listeners.Count == 0)
                    return;

                Action[] listenersSnapshot = listeners.ToArray();
                for (int index = 0; index < listenersSnapshot.Length; ++index)
                {
                    Action listener = listenersSnapshot[index];
                    try
                    {
                        listener();
                    }
                    catch (Exception exception)
                    {
                        Console.WriteLine($"[C# Error] Button.OnClick listener threw exception: {exception.Message}");
                    }
                }
            }
        }

        private static ButtonClickedEvent GetOrCreateClickedEvent(ulong entityIdentifier)
        {
            if (!ClickedEventsByEntityIdentifier.TryGetValue(entityIdentifier, out ButtonClickedEvent? clickedEvent)
                || clickedEvent == null)
            {
                clickedEvent = new ButtonClickedEvent();
                ClickedEventsByEntityIdentifier[entityIdentifier] = clickedEvent;
            }

            return clickedEvent;
        }

        internal static void DispatchOnClick(ulong entityIdentifier)
        {
            if (ClickedEventsByEntityIdentifier.TryGetValue(entityIdentifier, out ButtonClickedEvent? clickedEvent)
                && clickedEvent != null)
            {
                clickedEvent.InvokeListeners();
            }
        }

        internal static void ClearOnClick(ulong entityIdentifier)
        {
            ClickedEventsByEntityIdentifier.Remove(entityIdentifier);
        }

        internal static void ClearAllOnClick()
        {
            ClickedEventsByEntityIdentifier.Clear();
        }
    }

    public class SoundPlayer : Component
    {
        public ulong SoundHandle
        {
            get => InternalCalls.SoundPlayer_GetSoundHandle?.Invoke(Entity.ID) ?? 0UL;
            set => InternalCalls.SoundPlayer_SetSoundHandle?.Invoke(Entity.ID, value);
        }

        public float Volume
        {
            get => InternalCalls.SoundPlayer_GetVolume?.Invoke(Entity.ID) ?? 0.0f;
            set => InternalCalls.SoundPlayer_SetVolume?.Invoke(Entity.ID, value);
        }

        public bool Mute
        {
            get => InternalCalls.SoundPlayer_GetMute?.Invoke(Entity.ID) != 0;
            set => InternalCalls.SoundPlayer_SetMute?.Invoke(Entity.ID, (byte)(value ? 1 : 0));
        }

        public bool Loop
        {
            get => InternalCalls.SoundPlayer_GetLoop?.Invoke(Entity.ID) != 0;
            set => InternalCalls.SoundPlayer_SetLoop?.Invoke(Entity.ID, (byte)(value ? 1 : 0));
        }

        public bool PlayOnStart
        {
            get => InternalCalls.SoundPlayer_GetPlayOnStart?.Invoke(Entity.ID) != 0;
            set => InternalCalls.SoundPlayer_SetPlayOnStart?.Invoke(Entity.ID, (byte)(value ? 1 : 0));
        }

        public void Play() => InternalCalls.SoundPlayer_Play?.Invoke(Entity.ID);
        public void Stop() => InternalCalls.SoundPlayer_Stop?.Invoke(Entity.ID);
        public void Pause() => InternalCalls.SoundPlayer_Pause?.Invoke(Entity.ID);
        public void Resume() => InternalCalls.SoundPlayer_Resume?.Invoke(Entity.ID);

        /// <summary>播放一次性音效。soundHandle 为 0 时使用组件当前 SoundHandle。</summary>
        public void PlayOneShot(ulong soundHandle = 0UL) =>
            InternalCalls.SoundPlayer_PlayOneShot?.Invoke(Entity.ID, soundHandle);
    }

    public class SpriteRenderer : Component
    {
        public Vector4 Color
        {
            get
            {
                InternalCalls.SpriteRenderer_GetColor(Entity.ID, out Vector4 result);
                return result;
            }
            set => InternalCalls.SpriteRenderer_SetColor(Entity.ID, ref value);
        }

        /// <summary>子 Sprite 资产句柄（推荐）。</summary>
        public ulong SpriteAssetHandle
        {
            get => InternalCalls.SpriteRenderer_GetSpriteHandle(Entity.ID);
            set => InternalCalls.SpriteRenderer_SetSpriteHandle(Entity.ID, value);
        }

        /// <summary>与 SpriteAssetHandle 相同，保留兼容。</summary>
        public ulong SpriteHandle
        {
            get => SpriteAssetHandle;
            set => SpriteAssetHandle = value;
        }

        /// <summary>设置纹理时自动绑定其默认子 Sprite。</summary>
        public ulong TextureHandle
        {
            get => InternalCalls.SpriteRenderer_GetTextureHandle(Entity.ID);
            set => InternalCalls.SpriteRenderer_SetTextureHandle(Entity.ID, value);
        }

        /// <summary>水平镜像绘制，无需将 Transform.Scale.X 设为负数。</summary>
        public bool FlipHorizontal
        {
            get => InternalCalls.SpriteRenderer_GetFlipHorizontal(Entity.ID) != 0;
            set => InternalCalls.SpriteRenderer_SetFlipHorizontal(Entity.ID, value ? (byte)1 : (byte)0);
        }

        /// <summary>Sorting Layer 索引（数值越小越先绘制，位于后方）。</summary>
        public int SortingLayer
        {
            get => InternalCalls.SpriteRenderer_GetSortingLayer?.Invoke(Entity.ID) ?? 0;
            set => InternalCalls.SpriteRenderer_SetSortingLayer?.Invoke(Entity.ID, value);
        }

        /// <summary>同一 Sorting Layer 内的绘制顺序。</summary>
        public int SortingOrder
        {
            get => InternalCalls.SpriteRenderer_GetSortingOrder?.Invoke(Entity.ID) ?? 0;
            set => InternalCalls.SpriteRenderer_SetSortingOrder?.Invoke(Entity.ID, value);
        }
    }

    public class Camera : Component
    {
        public float OrthographicSize
        {
            get => InternalCalls.Camera_GetOrthographicSize?.Invoke(Entity.ID) ?? 10.0f;
            set => InternalCalls.Camera_SetOrthographicSize?.Invoke(Entity.ID, value);
        }

        public Vector4 BackgroundColor
        {
            get
            {
                Vector4 result = default;
                InternalCalls.Camera_GetBackgroundColor?.Invoke(Entity.ID, out result);
                return result;
            }
            set => InternalCalls.Camera_SetBackgroundColor?.Invoke(Entity.ID, ref value);
        }

        public bool Primary
        {
            get => InternalCalls.Camera_GetPrimary?.Invoke(Entity.ID) != 0;
            set => InternalCalls.Camera_SetPrimary?.Invoke(Entity.ID, value ? (byte)1 : (byte)0);
        }
    }

    public class SpriteAnimation : Component
    {
        public bool Playing
        {
            get => InternalCalls.SpriteAnimation_GetPlaying(Entity.ID) != 0;
            set => InternalCalls.SpriteAnimation_SetPlaying(Entity.ID, value ? (byte)1 : (byte)0);
        }

        public float FrameRate
        {
            get => InternalCalls.SpriteAnimation_GetFrameRate(Entity.ID);
            set => InternalCalls.SpriteAnimation_SetFrameRate(Entity.ID, value);
        }

        public string CurrentAnimation
        {
            get
            {
                if (InternalCalls.SpriteAnimation_GetCurrentAnimationName == null)
                    return string.Empty;
                IntPtr namePointer = InternalCalls.SpriteAnimation_GetCurrentAnimationName(Entity.ID);
                return namePointer == IntPtr.Zero
                    ? string.Empty
                    : Marshal.PtrToStringUTF8(namePointer) ?? string.Empty;
            }
            set
            {
                if (InternalCalls.SpriteAnimation_SetCurrentAnimationName == null
                    || string.IsNullOrEmpty(value))
                    return;
                IntPtr namePointer = Marshal.StringToCoTaskMemUTF8(value);
                InternalCalls.SpriteAnimation_SetCurrentAnimationName(Entity.ID, namePointer);
                Marshal.FreeCoTaskMem(namePointer);
            }
        }

        public void Play()
        {
            InternalCalls.SpriteAnimation_Play?.Invoke(Entity.ID, IntPtr.Zero);
        }

        public void Play(string animationName)
        {
            if (InternalCalls.SpriteAnimation_Play == null)
                return;
            if (string.IsNullOrEmpty(animationName))
            {
                Play();
                return;
            }

            IntPtr namePointer = Marshal.StringToCoTaskMemUTF8(animationName);
            InternalCalls.SpriteAnimation_Play(Entity.ID, namePointer);
            Marshal.FreeCoTaskMem(namePointer);
        }

        public void Stop()
        {
            InternalCalls.SpriteAnimation_SetPlaying(Entity.ID, 0);
        }
    }

    public class Rigidbody2D : Component
    {
        public Rigidbody2DBodyType BodyType
        {
            get => (Rigidbody2DBodyType)(InternalCalls.Rigidbody2D_GetBodyType?.Invoke(Entity.ID) ?? 0);
            set => InternalCalls.Rigidbody2D_SetBodyType?.Invoke(Entity.ID, (int)value);
        }

        public bool FixedRotation
        {
            get => InternalCalls.Rigidbody2D_GetFixedRotation?.Invoke(Entity.ID) != 0;
            set => InternalCalls.Rigidbody2D_SetFixedRotation?.Invoke(Entity.ID, value ? (byte)1 : (byte)0);
        }

        public Vector2 Velocity
        {
            get
            {
                InternalCalls.Rigidbody2D_GetLinearVelocity(Entity.ID, out Vector2 result);
                return result;
            }
            set => InternalCalls.Rigidbody2D_SetLinearVelocity(Entity.ID, ref value);
        }

        public void ApplyImpulse(Vector2 impulse, Vector2 point, bool wake)
        {
            InternalCalls.Rigidbody2D_ApplyLinearImpulse(Entity.ID, ref impulse, ref point, wake);
        }

        public void ApplyImpulse(Vector2 impulse, bool wake)
        {
            InternalCalls.Rigidbody2D_ApplyLinearImpulseToCenter(Entity.ID, ref impulse, wake);
        }
    }

    public class BoxCollider2D : Component
    {
        public Vector2 Offset
        {
            get
            {
                InternalCalls.BoxCollider2D_GetOffset(Entity.ID, out Vector2 result);
                return result;
            }
            set => InternalCalls.BoxCollider2D_SetOffset(Entity.ID, ref value);
        }

        public Vector2 Size
        {
            get
            {
                InternalCalls.BoxCollider2D_GetSize(Entity.ID, out Vector2 result);
                return result;
            }
            set => InternalCalls.BoxCollider2D_SetSize(Entity.ID, ref value);
        }

        public float Density
        {
            get => InternalCalls.BoxCollider2D_GetDensity(Entity.ID);
            set => InternalCalls.BoxCollider2D_SetDensity(Entity.ID, value);
        }

        public float Friction
        {
            get => InternalCalls.BoxCollider2D_GetFriction(Entity.ID);
            set => InternalCalls.BoxCollider2D_SetFriction(Entity.ID, value);
        }

        public float Restitution
        {
            get => InternalCalls.BoxCollider2D_GetRestitution(Entity.ID);
            set => InternalCalls.BoxCollider2D_SetRestitution(Entity.ID, value);
        }

        public bool IsTrigger
        {
            get => InternalCalls.BoxCollider2D_GetIsTrigger?.Invoke(Entity.ID) != 0;
            set => InternalCalls.BoxCollider2D_SetIsTrigger?.Invoke(Entity.ID, value ? (byte)1 : (byte)0);
        }

        public int Layer
        {
            get => InternalCalls.BoxCollider2D_GetLayer?.Invoke(Entity.ID) ?? 0;
            set => InternalCalls.BoxCollider2D_SetLayer?.Invoke(Entity.ID, value);
        }
    }

    public class CircleCollider2D : Component
    {
        public Vector2 Offset
        {
            get
            {
                InternalCalls.CircleCollider2D_GetOffset(Entity.ID, out Vector2 result);
                return result;
            }
            set => InternalCalls.CircleCollider2D_SetOffset(Entity.ID, ref value);
        }

        public float Radius
        {
            get => InternalCalls.CircleCollider2D_GetRadius(Entity.ID);
            set => InternalCalls.CircleCollider2D_SetRadius(Entity.ID, value);
        }

        public float Density
        {
            get => InternalCalls.CircleCollider2D_GetDensity(Entity.ID);
            set => InternalCalls.CircleCollider2D_SetDensity(Entity.ID, value);
        }

        public float Friction
        {
            get => InternalCalls.CircleCollider2D_GetFriction(Entity.ID);
            set => InternalCalls.CircleCollider2D_SetFriction(Entity.ID, value);
        }

        public float Restitution
        {
            get => InternalCalls.CircleCollider2D_GetRestitution(Entity.ID);
            set => InternalCalls.CircleCollider2D_SetRestitution(Entity.ID, value);
        }

        public bool IsTrigger
        {
            get => InternalCalls.CircleCollider2D_GetIsTrigger?.Invoke(Entity.ID) != 0;
            set => InternalCalls.CircleCollider2D_SetIsTrigger?.Invoke(Entity.ID, value ? (byte)1 : (byte)0);
        }

        public int Layer
        {
            get => InternalCalls.CircleCollider2D_GetLayer?.Invoke(Entity.ID) ?? 0;
            set => InternalCalls.CircleCollider2D_SetLayer?.Invoke(Entity.ID, value);
        }
    }
}