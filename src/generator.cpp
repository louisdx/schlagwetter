#include "generator.h"
#include "random.h"
#include "cmdlineoptions.h"
#include "map.h"
#include "packetcrafter.h"

std::shared_ptr<NoiseGenerator> pNG;

NoiseGenerator::NoiseGenerator(int mapseed, bool addCaveLava, unsigned int caveSize, double caveThreshold)
  : caveNoise(),
    ridgedMultiNoise(),
    m_addCaveLava(addCaveLava),
    m_caveSize(caveSize),
    m_caveThreshold(caveThreshold)
{
  int seed = 137337;
  seed = mapseed;

  if (seed != -1)
  {
    std::cout << "Global map seed set by user (" << std::dec << (unsigned int)(seed) << "), thank you." << std::endl;
  }
  else
  {
    seed = uniformUINT32();
    std::cout << "Global map seed not set, generating: " << std::dec << (unsigned int)(seed) << std::endl;
  }

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
      uint8_t currentHeight = (uint8_t)((pNG->ridgedMultiNoise.GetValue(bX + 16 * cX(cc), 0, bZ + 16 * cZ(cc)) * 15) + 64);

      const uint8_t stoneHeight = (currentHeight * 94) / 100;
      const uint8_t ymax = std::max(currentHeight, sea_level);

      for (size_t bY = 0; bY <= ymax; bY++)
      {
        const LocalCoords lc(bX, bY, bZ);

        // Place bedrock
        if (bY == 0)
        {
          c.blockType(lc) = BLOCK_Bedrock;
          continue;
        }

        if (bY < currentHeight)
        {
          if (bY < stoneHeight)
          {
            c.blockType(lc) = BLOCK_Stone;

            // Add caves
            if (add_caves) pNG->addCaves(c.blockType(lc), getWorldCoords(lc, cc));
          }
          else
          {
            c.blockType(lc) = BLOCK_Dirt;
          }
        }
        else if (bY == currentHeight)
        {
          if (bY == sea_level || bY == sea_level - 1 || bY == sea_level - 2)
          {
            c.blockType(lc) = BLOCK_Sand;  // FF
          }
          else if (bY < sea_level - 1)
          {
            c.blockType(lc) = BLOCK_Gravel;  // FF
          }
          else
          {
            c.blockType(lc) = topBlock;  // FF
          }
        }
        else
        {
          if (bY <= sea_level)
          {
            c.blockType(lc) = BLOCK_Water;  // FF
          }
          else
          {
            c.blockType(lc) = BLOCK_Air;  // FF
          }
        }
      }
    }
  }
}
