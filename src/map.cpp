#include <iostream>
#include <zlib.h>
#include "constants.h"

#include "map.h"
#include "generator.h"
#include "packetcrafter.h"

std::string Chunk::compress() const
{
  unsigned long int outlength = compressBound(size());
  unsigned char * buf = new unsigned char[outlength];
  int zres = ::compress(buf, &outlength, m_data.data(), size());
  if (zres != Z_OK)
  {
    std::cerr << "Error during zlib deflate!" << std::endl;
    delete[] buf;
    return "";
  }
  else
  {
    std::cout << "zlib deflate: " << std::dec << outlength << std::endl;
    std::string r(reinterpret_cast<char*>(buf), outlength);
    delete[] buf;
    return r;
  }
}

Map::Map()
  :
  m_chunkMap()
{
}

const Chunk & Map::getChunkOrGnerateNew(const ChunkCoords & cc)
{
  ChunkMap::iterator ins = m_chunkMap.find(cc);

  if (ins == m_chunkMap.end())
  {
    ins = m_chunkMap.insert(ChunkMap::value_type(cc, std::make_shared<Chunk>())).first;
    generateWithNoise(*ins->second, cc);
  }

  return *ins->second;
}
