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

  inline void addCaves(uint8_t & block, const WorldCoords & wc)
  {
    if (caveNoise.GetValue(wX(wc) / 4.0, wY(wc) / 1.5, wZ(wc) / 4.0) > m_caveThreshold)
      block = (wY(wc) < 10 && m_addCaveLava) ? BLOCK_Lava : BLOCK_Air;
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
