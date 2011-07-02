#include <fstream>
#include <iostream>
#include <limits>
#include <ctime>

#include "random.h"
#include "generator.h"

uint32_t seedval = 0;

MyRNG prng;

std::uniform_int_distribution<uint32_t> m_uniformUINT32(0, std::numeric_limits<uint32_t>::max());

void initPRNG(int mapseed)
{
  std::ifstream urandom("/dev/urandom");
  if (urandom)
  {
    urandom.read(reinterpret_cast<char*>(&seedval), sizeof(seedval));
  }
  else
  {
    seedval = std::time(NULL);
  }

  std::cout << "Seeding the PRNG with: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << seedval << std::endl;

  prng.seed(seedval);

  pNG.reset(new NoiseGenerator(mapseed));
}
