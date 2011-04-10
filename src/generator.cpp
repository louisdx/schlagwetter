#include "generator.h"
#include "map.h"
#include "packetcrafter.h"

NoiseGenerator NG;

NoiseGenerator::NoiseGenerator(int seed, bool addCaveLava, unsigned int caveSize, double caveThreshold)
  : caveNoise(),
    ridgedMultiNoise(),
    m_addCaveLava(addCaveLava),
    m_caveSize(caveSize),
    m_caveThreshold(caveThreshold)
{
  caveNoise.SetSeed(seed + 22);
  caveNoise.SetFrequency(1.0 / m_caveSize);
  caveNoise.SetOctaveCount(4);

  ridgedMultiNoise.SetSeed(seed);
  ridgedMultiNoise.SetOctaveCount(6);
  ridgedMultiNoise.SetFrequency(1.0 / 180.0);
  ridgedMultiNoise.SetLacunarity(2.0);
}

void generateWithNoise(Chunk & c, const ChunkCoords & cc)
{
  const bool winter_enabled = false, add_caves = true;
  const uint8_t sea_level = 64;

  // Winterland or Summerland
  const EBlockItem topBlock = winter_enabled ? BLOCK_Snow : BLOCK_Grass;

  // Populate blocks in chunk
 
  //std::array<uint8_t, 16 * 16> heightmap;

  for (int bX = 0; bX < 16; ++bX)
  {
    for (int bZ = 0; bZ < 16; ++bZ)
    {
      //heightmap[(bZ << 4) + bX] = currentHeight = (uint8_t)((NG.ridgedMultiNoise.GetValue(bX + 16 * cX(cc), 0, bZ + 16 * cZ(cc)) * 15) + 64);
      uint8_t currentHeight = (uint8_t)((NG.ridgedMultiNoise.GetValue(bX + 16 * cX(cc), 0, bZ + 16 * cZ(cc)) * 15) + 64);

      const uint8_t stoneHeight = (currentHeight * 94) / 100;
      const uint8_t ymax = std::max(currentHeight, sea_level);

      for (size_t bY = 0; bY <= ymax; bY++)
      {
        // Place bedrock
        if (bY == 0)
        {
          c.blockType(bX, bY, bZ) = BLOCK_Bedrock;
          continue;
        }

        if (bY < currentHeight)
        {
          if (bY < stoneHeight)
          {
            c.blockType(bX, bY, bZ) = BLOCK_Stone;

            // Add caves
            if (add_caves) NG.addCaves(c.blockType(bX, bY, bZ), bX + 16 * cX(cc), bY, bZ + 16 * cZ(cc));
          }
          else
          {
            c.blockType(bX, bY, bZ) = BLOCK_Dirt;
          }
        }
        else if (bY == currentHeight)
        {
          if (bY == sea_level || bY == sea_level - 1 || bY == sea_level - 2)
          {
            c.blockType(bX, bY, bZ) = BLOCK_Sand;  // FF
          }
          else if (bY < sea_level - 1)
          {
            c.blockType(bX, bY, bZ) = BLOCK_Gravel;  // FF
          }
          else
          {
            c.blockType(bX, bY, bZ) = topBlock;  // FF
          }
        }
        else
        {
          if (bY <= sea_level)
          {
            c.blockType(bX, bY, bZ) = BLOCK_Water;  // FF
          }
          else
          {
            c.blockType(bX, bY, bZ) = BLOCK_Air;  // FF
          }
        }
      }
    }
  }
}
