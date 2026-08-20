namespace WildStar.Archive;

public readonly struct WsBlock
{
    public readonly ulong Offset;
    public readonly ulong Size;

    public WsBlock(ulong offset, ulong size)
    {
        Offset = offset;
        Size = size;
    }
}
