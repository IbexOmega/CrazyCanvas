vec4 ChannelBitMult(uint mask, uint dstChannel)
{
    uint shift = (3 - dstChannel) * 4;
    return vec4(
        float((mask >> (shift + 3)) & 0x00000001), 
        float((mask >> (shift + 2)) & 0x00000001), 
        float((mask >> (shift + 1)) & 0x00000001), 
        float((mask >> (shift + 0)) & 0x00000001));
}  