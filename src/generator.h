#ifndef H_GENERATOR
#define H_GENERATOR

#include <cstdint>
#include <libnoise/noise.h>
#include "constants.h"
#include "types.h"

class NoiseGenerator
{
public:
  explicit NoiseGenerator(int seed = 137337, bool addCaveLava = true, unsigned int caveSize = 30, double caveThreshold = 0.5);

  inline void addCaves(uint8_t & block, int32_t wX, int32_t wY, int32_t wZ)
  {
    if (caveNoise.GetValue(wX / 4.0, wY / 1.5, wZ / 4.0) > m_caveThreshold)
      block = (wY < 10 && m_addCaveLava) ? BLOCK_LAVA : BLOCK_AIR;
  }

  noise::module::RidgedMulti caveNoise;
  noise::module::RidgedMulti ridgedMultiNoise;

private:
  bool m_addCaveLava;
  int m_caveSize;
  double m_caveThreshold;
};

extern NoiseGenerator NG;

class Chunk;

void generateWithNoise(Chunk & c, const ChunkCoords & cc);


#endif
