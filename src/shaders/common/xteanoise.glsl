#ifndef XTEANOISE_GLSL
#define XTEANOISE_GLSL

// XTEA algorithm (public domain) https://www.cix.co.uk/~klockstone/xtea.pdf

uint CalculateSeedXTEA(uint seed0, uint seed1)
{
	const uint num_rounds = 16;
	const uint key[4] = {0x44DFD203, 0x7D5C4117, 0xAAC42FA6, 0x6F94B357};
	uint sum = 0;
	uint delta = 0x9E3779B9;
	for (uint i = 0; i < num_rounds; i++)
	{
        seed0 += (((seed1 << 4) ^ (seed1 >> 5)) + seed1) ^ (sum + key[sum & 3]);
        sum += delta;
        seed1 += (((seed0 << 4) ^ (seed0 >> 5)) + seed0) ^ (sum + key[(sum>>11) & 3]);
    }
    return seed0 + seed1;
}

uint CalculateSeedXTEA(ivec2 texelPos, uint seed)
{
    return CalculateSeedXTEA((texelPos.x << 16) | texelPos.y, seed);
}

// only works until 1024 for each component
// [10bits x] 0 [10bits y] 0 [10 bits z]
//            ^ bit 21     ^ bit 10
uint CalculateSeedXTEA(uvec3 voxel, uint seed) {
	const uint mask = (1 << 10) - 1;
	return CalculateSeedXTEA((voxel.x & mask) << 22 | (voxel.y & mask) << 11 | (voxel.z & mask), seed);
}

#endif // XTEANOISE_GLSL
