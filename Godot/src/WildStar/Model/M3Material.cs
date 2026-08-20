namespace WildStar.Model;

public readonly struct M3MaterialLayer
{
    public M3MaterialLayer(int textureA, int textureB)
    {
        TextureA = textureA;
        TextureB = textureB;
    }

    public int TextureA { get; }

    public int TextureB { get; }
}

public sealed class M3Material
{
    public M3Material(M3MaterialLayer[] layers)
    {
        Layers = layers;
    }

    public M3MaterialLayer[] Layers { get; }
}

public sealed class M3Texture
{
    public M3Texture(int slot, int selector, uint flags, string path)
    {
        Slot = slot;
        Selector = selector;
        Flags = flags;
        Path = path;
    }

    public int Slot { get; }

    public int Selector { get; }

    public uint Flags { get; }

    public string Path { get; }
}
